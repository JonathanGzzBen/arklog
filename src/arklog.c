#include "arklog/arklog.h"
#include "arklog/ring_buffer.h"
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
  const size_t log_details_length =
      snprintf(logger->memory + sizeof(size_t), logger->max_message_length,
               "[LEVEL %5s] [%s:%d] [FUNC: %s] ", debug_types_str[level], file,
               line, func);
  va_list fmt_args;
  va_start(fmt_args, fmt);
  const size_t log_message_length = vsnprintf(
      logger->memory + sizeof(size_t) + (log_details_length * sizeof(char)),
      logger->max_message_length - log_details_length, fmt, fmt_args);
  va_end(fmt_args);
  logger->memory[sizeof(size_t) + log_details_length + log_message_length] =
      '\n';
  const size_t message_length =
      log_details_length + log_message_length + 1; // Line break

  memcpy(logger->memory, &message_length, sizeof(size_t));

  assert(0 < message_length);
  assert(message_length < logger->max_message_length);
  assert(message_length < logger->ring_buffer.elem_size);

  pthread_mutex_lock(&logger->queue_lock);
  alog_ring_buffer_push(
      &logger->ring_buffer,
      logger->memory); // Ignore even if push fails because queue was full
  pthread_mutex_unlock(&logger->queue_lock);
}

AlogLogger alog_logger_create(AlogLoggerConfiguration configuration) {
  AlogLogger result = {.memory = NULL,
                       .ring_buffer = NULL,
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

  AlogRingBuffer ring_buffer =
      alog_ring_buffer_create(configuration.queue_size, log_size);

  assert(configuration.sink != NULL);
  result.sink = configuration.sink;
  result.ring_buffer = ring_buffer;
  result.max_message_length = configuration.max_message_length;
  result.stop_flag = false;
  result.current_log_level = configuration.initial_log_level;
  result.valid = true;
  return result;
}

static void *alog_logger_flush_continuous(void *logger) {
  AlogLogger *alog_logger = logger;
  while (!alog_logger->stop_flag) {
    pthread_mutex_lock(&alog_logger->queue_lock);
    if (!alog_ring_buffer_pop(&alog_logger->ring_buffer, alog_logger->memory)) {
      // If queue was empty
      pthread_mutex_unlock(&alog_logger->queue_lock);
      // TODO: Define a configurable polling rate even when not empty
      struct timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = 100000000L; // 100ms
      nanosleep(&ts, NULL);
      continue;
    }
    pthread_mutex_unlock(&alog_logger->queue_lock);
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
  while (!alog_ring_buffer_is_empty(logger->ring_buffer)) {
    pthread_mutex_lock(&logger->queue_lock);
    assert(alog_ring_buffer_pop(&logger->ring_buffer, logger->memory));
    pthread_mutex_unlock(&logger->queue_lock);
    size_t message_size = 0;
    memcpy(&message_size, logger->memory, sizeof(size_t));
    fwrite(logger->memory + sizeof(size_t), message_size, 1, logger->sink);
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
  alog_ring_buffer_free(&logger->ring_buffer);
  free(logger->memory);
  logger->valid = false;
}
