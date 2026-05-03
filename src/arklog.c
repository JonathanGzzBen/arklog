#include "arklog/arklog.h"
#include "arklog/locked_queue.h"
#include "arklog/lockfree_queue.h"
#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static const char *debug_types_str[] = {"FATAL", "ERROR", "WARN",
                                        "INFO",  "DEBUG", "TRACE"};

void alog_log(AlogLogger *logger, int level, const char *file, int line,
              const char *func, const char *fmt, ...) {
  assert(logger != NULL);
  if (logger->stop_flag || logger->current_log_level < level)
    return;
  // Example of desired output
  // [2024-12-09 15:30:45.123456] [Thread-12345] [INFO] [main.c:42] Application
  // started
  // Pending: Timestamp, Thread ID, debug level in text
  assert(0 <= level && level < 6);
  const size_t log_header_length =
      snprintf(logger->memory + sizeof(size_t), logger->max_message_length,
               "[LEVEL %5s] [%s:%d] [FUNC: %s] ", debug_types_str[level], file,
               line, func);
  va_list fmt_args;
  va_start(fmt_args, fmt);
  size_t bytes_to_write = (log_header_length < logger->max_message_length)
                              ? log_header_length
                              : logger->max_message_length;

  size_t log_message_length = 0;
  if (bytes_to_write < logger->max_message_length) {
    log_message_length = vsnprintf(
        logger->memory + sizeof(size_t) + (log_header_length * sizeof(char)),
        logger->max_message_length - log_header_length, fmt, fmt_args);
    va_end(fmt_args);

    bytes_to_write += log_message_length;
    bytes_to_write =
        (bytes_to_write < logger->max_message_length)
            ? bytes_to_write
            : logger->max_message_length - 1; // Make space for line break
  }

  // Write size of log into memory
  logger->memory[sizeof(size_t) + (bytes_to_write * sizeof(char))] = '\n';
  bytes_to_write++; // Line break
  memcpy(logger->memory, &bytes_to_write, sizeof(size_t));

  assert(0 < bytes_to_write);
  assert(bytes_to_write <= (logger->max_message_length));
  assert(bytes_to_write <
         sizeof(size_t) + logger->max_message_length + sizeof(char));

  if (logger->queue_type == ALOG_QUEUE_LOCKFREE_MPMC) {
    alog_lockfree_queue_push(&logger->queue.lockfree, logger->memory);
  } else {
    pthread_mutex_lock(&logger->queue_lock);
    alog_locked_queue_push(&logger->queue.locked, logger->memory);
    pthread_mutex_unlock(&logger->queue_lock);
  }
}

AlogLogger alog_logger_create(AlogLoggerConfiguration configuration) {
  AlogLogger result = {.queue_type = ALOG_QUEUE_MUTEX_LOCKED,
                       .memory = NULL,
                       .sink = NULL,
                       .max_message_length = 0,
                       .flushing_thread = 0,
                       .queue_lock = PTHREAD_MUTEX_INITIALIZER,
                       .current_log_level = LOG_LEVEL_FATAL,
                       .valid = false};

  if (configuration.sink == NULL) {
    return result;
  }

  assert(LOG_LEVEL_FATAL <= configuration.initial_log_level &&
         configuration.initial_log_level <= LOG_LEVEL_TRACE);
  if (configuration.initial_log_level < LOG_LEVEL_FATAL ||
      LOG_LEVEL_TRACE < configuration.initial_log_level) {
    return result;
  }

  const size_t log_size = sizeof(size_t) +
                          (configuration.max_message_length * sizeof(char)) +
                          sizeof(char); // Null char at end
  result.memory = malloc(log_size);
  assert(result.memory != NULL);
  if (result.memory == NULL) {
    return result;
  }

  assert(configuration.sink != NULL);
  result.sink = configuration.sink;
  result.max_message_length = configuration.max_message_length;
  result.stop_flag = false;
  result.queue_type = configuration.queue_type;
  result.current_log_level = configuration.initial_log_level;

  if (configuration.queue_type == ALOG_QUEUE_LOCKFREE_MPMC) {
    result.queue.lockfree =
        alog_lockfree_queue_create(configuration.queue_size, log_size);
  } else {
    result.queue.locked =
        alog_locked_queue_create(configuration.queue_size, log_size);
  }

  result.valid = true;
  return result;
}

static void *alog_logger_flush_continuous(void *logger) {
  AlogLogger *alog_logger = logger;
  while (!alog_logger->stop_flag) {
    bool popped;
    if (alog_logger->queue_type == ALOG_QUEUE_LOCKFREE_MPMC) {
      popped = alog_lockfree_queue_pop(&alog_logger->queue.lockfree,
                                       alog_logger->memory);
    } else {
      pthread_mutex_lock(&alog_logger->queue_lock);
      popped = alog_locked_queue_pop(&alog_logger->queue.locked,
                                     alog_logger->memory);
      pthread_mutex_unlock(&alog_logger->queue_lock);
    }
    if (!popped) {
      // TODO: Define a configurable polling rate even when not empty
      struct timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = 100000000L; // 100ms
      nanosleep(&ts, NULL);
      continue;
    }
    size_t message_size = 0;
    memcpy(&message_size, alog_logger->memory, sizeof(size_t));
    fwrite(alog_logger->memory + sizeof(size_t), message_size, 1,
           alog_logger->sink);
  }
  alog_logger_flush(logger);
  return NULL;
}

void alog_logger_start_flushing_thread(AlogLogger *logger) {
  assert(logger->flushing_thread == 0);
  pthread_create(&logger->flushing_thread, NULL, alog_logger_flush_continuous,
                 logger);
}

void alog_logger_flush(AlogLogger *logger) {
  if (logger->queue_type == ALOG_QUEUE_LOCKFREE_MPMC) {
    while (alog_lockfree_queue_pop(&logger->queue.lockfree, logger->memory)) {
      size_t message_size = 0;
      memcpy(&message_size, logger->memory, sizeof(size_t));
      fwrite(logger->memory + sizeof(size_t), message_size, 1, logger->sink);
    }
  } else {
    pthread_mutex_lock(&logger->queue_lock);
    while (alog_locked_queue_pop(&logger->queue.locked, logger->memory)) {
      pthread_mutex_unlock(&logger->queue_lock);
      size_t message_size = 0;
      memcpy(&message_size, logger->memory, sizeof(size_t));
      fwrite(logger->memory + sizeof(size_t), message_size, 1, logger->sink);
      pthread_mutex_lock(&logger->queue_lock);
    }
    pthread_mutex_unlock(&logger->queue_lock);
  }
  fflush(logger->sink);
}

void alog_logger_set_log_level(AlogLogger *logger, LogLevel log_level) {
  logger->current_log_level = log_level;
}

void alog_logger_free(AlogLogger *logger) {
  assert(logger->valid);
  if (logger->flushing_thread != 0) {
    logger->stop_flag = true;
    pthread_join(logger->flushing_thread, NULL);
  }
  if (logger->queue_type == ALOG_QUEUE_LOCKFREE_MPMC) {
    alog_lockfree_queue_free(&logger->queue.lockfree);
  } else {
    alog_locked_queue_free(&logger->queue.locked);
  }
  free(logger->memory);
  logger->valid = false;
}
