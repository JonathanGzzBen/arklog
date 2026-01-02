#include "arklog/arklog.h"
#include "arklog/ring_buffer.h"
#include <assert.h>
#include <stdio.h>

void alog_log(int level, const char *file, int line, const char *func,
              const char *fmt, ...) {
  // Example of desired output
  // [2024-12-09 15:30:45.123456] [Thread-12345] [INFO] [main.c:42] Application
  // started
  // Pending: Timestamp, Thread ID, debug level in text
  printf("[LEVEL %d] [%s:%d] [FUNC: %s] %s\n", level, file, line, func, fmt);
}

AlogLogger alog_logger_create(AlogLoggerConfiguration configuration) {
  AlogLogger result = {.memory = NULL, .ring_buffer = NULL, .valid = false};

  if (configuration.sink == NULL) {
    return result;
  }

  const size_t memory_amount = configuration.queue_size *
                               (configuration.max_message_length + 1) *
                               sizeof(char);
  result.memory = malloc(memory_amount);
  assert(result.memory != NULL);
  if (result.memory == NULL) {
    return result;
  }

  AlogRingBuffer ring_buffer = alog_ring_buffer_create(
      configuration.queue_size,
      (configuration.max_message_length + 1) * sizeof(char));

  result.valid = true;
  return result;
}

void alog_logger_free(AlogLogger *logger) {
  assert(logger->valid);
  alog_ring_buffer_free(logger->ring_buffer);
  free(logger->memory);
  logger->valid = false;
}
