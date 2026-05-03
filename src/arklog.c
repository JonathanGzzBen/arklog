#include "arklog/arklog.h"
#include "arklog/lockfree_mpmc_queue.h"
#include "arklog/lockfree_mpsc_queue.h"
#include "arklog/mutex_locked_queue.h"
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

  // Each calling thread gets its own buffer
  _Thread_local static char tl_buf[sizeof(size_t) + ALOG_MAX_MESSAGE_LENGTH + 1];
  const size_t msg_max = logger->max_message_length < ALOG_MAX_MESSAGE_LENGTH
                             ? logger->max_message_length
                             : ALOG_MAX_MESSAGE_LENGTH;

  const size_t log_header_length =
      snprintf(tl_buf + sizeof(size_t), msg_max,
               "[LEVEL %5s] [%s:%d] [FUNC: %s] ", debug_types_str[level], file,
               line, func);
  va_list fmt_args;
  va_start(fmt_args, fmt);
  size_t bytes_to_write = (log_header_length < msg_max)
                              ? log_header_length
                              : msg_max;

  size_t log_message_length = 0;
  if (bytes_to_write < msg_max) {
    log_message_length =
        vsnprintf(tl_buf + sizeof(size_t) + log_header_length,
                  msg_max - log_header_length, fmt, fmt_args);
    va_end(fmt_args);

    bytes_to_write += log_message_length;
    bytes_to_write = (bytes_to_write < msg_max)
                         ? bytes_to_write
                         : msg_max - 1; // Make space for line break
  }

  tl_buf[sizeof(size_t) + bytes_to_write] = '\n';
  bytes_to_write++;
  memcpy(tl_buf, &bytes_to_write, sizeof(size_t));

  assert(0 < bytes_to_write);
  assert(bytes_to_write <= msg_max);
  assert(bytes_to_write < sizeof(size_t) + msg_max + sizeof(char));

  if (logger->queue_type == ALOG_QUEUE_LOCKFREE_MPMC) {
    alog_lockfree_mpmc_queue_push(&logger->queue.lockfree_mpmc, tl_buf);
  } else if (logger->queue_type == ALOG_QUEUE_LOCKFREE_MPSC) {
    alog_lockfree_mpsc_queue_push(&logger->queue.lockfree_mpsc, tl_buf);
  } else {
    pthread_mutex_lock(&logger->queue_lock);
    alog_mutex_locked_queue_push(&logger->queue.mutex_locked, tl_buf);
    pthread_mutex_unlock(&logger->queue_lock);
  }
}

AlogLogger alog_logger_create(AlogLoggerConfiguration configuration) {
  AlogLogger result = {.queue_type = ALOG_QUEUE_MUTEX_LOCKED,
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

  // Queue slot size is fixed to the compile-time cap so it matches tl_buf in
  // alog_log and pop_buf in the flush functions.
  const size_t log_size = sizeof(size_t) + ALOG_MAX_MESSAGE_LENGTH + 1;

  assert(configuration.sink != NULL);
  result.sink = configuration.sink;
  result.max_message_length = configuration.max_message_length;
  result.stop_flag = false;
  result.queue_type = configuration.queue_type;
  result.current_log_level = configuration.initial_log_level;

  if (configuration.queue_type == ALOG_QUEUE_LOCKFREE_MPMC) {
    result.queue.lockfree_mpmc =
        alog_lockfree_mpmc_queue_create(configuration.queue_size, log_size);
  } else if (configuration.queue_type == ALOG_QUEUE_LOCKFREE_MPSC) {
    result.queue.lockfree_mpsc =
        alog_lockfree_mpsc_queue_create(configuration.queue_size, log_size);
  } else {
    result.queue.mutex_locked =
        alog_mutex_locked_queue_create(configuration.queue_size, log_size);
  }

  result.valid = true;
  return result;
}

static void *alog_logger_flush_continuous(void *logger) {
  AlogLogger *alog_logger = logger;
  char pop_buf[sizeof(size_t) + ALOG_MAX_MESSAGE_LENGTH + 1];
  while (!alog_logger->stop_flag) {
    bool popped;
    if (alog_logger->queue_type == ALOG_QUEUE_LOCKFREE_MPMC) {
      popped = alog_lockfree_mpmc_queue_pop(&alog_logger->queue.lockfree_mpmc,
                                            pop_buf);
    } else if (alog_logger->queue_type == ALOG_QUEUE_LOCKFREE_MPSC) {
      popped = alog_lockfree_mpsc_queue_pop(&alog_logger->queue.lockfree_mpsc,
                                            pop_buf);
    } else {
      pthread_mutex_lock(&alog_logger->queue_lock);
      popped = alog_mutex_locked_queue_pop(&alog_logger->queue.mutex_locked,
                                           pop_buf);
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
    memcpy(&message_size, pop_buf, sizeof(size_t));
    fwrite(pop_buf + sizeof(size_t), message_size, 1, alog_logger->sink);
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
  char pop_buf[sizeof(size_t) + ALOG_MAX_MESSAGE_LENGTH + 1];
  if (logger->queue_type == ALOG_QUEUE_LOCKFREE_MPMC) {
    while (alog_lockfree_mpmc_queue_pop(&logger->queue.lockfree_mpmc,
                                        pop_buf)) {
      size_t message_size = 0;
      memcpy(&message_size, pop_buf, sizeof(size_t));
      fwrite(pop_buf + sizeof(size_t), message_size, 1, logger->sink);
    }
  } else if (logger->queue_type == ALOG_QUEUE_LOCKFREE_MPSC) {
    while (alog_lockfree_mpsc_queue_pop(&logger->queue.lockfree_mpsc,
                                        pop_buf)) {
      size_t message_size = 0;
      memcpy(&message_size, pop_buf, sizeof(size_t));
      fwrite(pop_buf + sizeof(size_t), message_size, 1, logger->sink);
    }
  } else {
    pthread_mutex_lock(&logger->queue_lock);
    while (alog_mutex_locked_queue_pop(&logger->queue.mutex_locked, pop_buf)) {
      pthread_mutex_unlock(&logger->queue_lock);
      size_t message_size = 0;
      memcpy(&message_size, pop_buf, sizeof(size_t));
      fwrite(pop_buf + sizeof(size_t), message_size, 1, logger->sink);
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
    alog_lockfree_mpmc_queue_free(&logger->queue.lockfree_mpmc);
  } else if (logger->queue_type == ALOG_QUEUE_LOCKFREE_MPSC) {
    alog_lockfree_mpsc_queue_free(&logger->queue.lockfree_mpsc);
  } else {
    alog_mutex_locked_queue_free(&logger->queue.mutex_locked);
  }
  logger->valid = false;
}
