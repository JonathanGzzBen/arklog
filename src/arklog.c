#include "arklog/arklog.h"
#include "arklog/ring_buffer.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void alog_log(AlogLogger *logger, int level, const char *file, int line,
              const char *func, const char *fmt, ...) {
  // Example of desired output
  // [2024-12-09 15:30:45.123456] [Thread-12345] [INFO] [main.c:42] Application
  // started
  // Pending: Timestamp, Thread ID, debug level in text
  static char static_buf[1024];
  // memset(static_buf, 0, 1024);
  /**
   * TODO: Remover hardcodeo de longitud. Usar el static_buf cuando el mensaje
   * quepa ahi, de lo contrario, tener un buffer en heap con espacio extra y que
   * vaya creciendo conforme se necesite. Asi es dinamico y se evitan memory
   * allocations en el hot path. Preferible en el push solo copiar esa cantidad
   * de bytes para que no sea una vulnerabilidad
   */
  int message_length = snprintf(static_buf, logger->max_message_length,
                                "[LEVEL %d] [%s:%d] [FUNC: %s] %s\n", level,
                                file, line, func, fmt);

  assert(0 < message_length);
  assert(message_length < logger->max_message_length);
  assert(message_length < logger->ring_buffer.elem_size);

  printf("%s", static_buf);

  Log log = {.length = message_length, .message = static_buf};

  assert(alog_ring_buffer_push(&logger->ring_buffer, &log));
}

AlogLogger alog_logger_create(AlogLoggerConfiguration configuration) {
  AlogLogger result = {.memory = NULL,
                       .ring_buffer = NULL,
                       .sink = NULL,
                       .max_message_length = 0,
                       .flushing_thread = 0,
                       .valid = false};

  if (configuration.sink == NULL) {
    return result;
  }

  size_t elem_size =
      sizeof(size_t) + (configuration.max_message_length * sizeof(char));

  const size_t memory_amount = configuration.queue_size * elem_size;
  result.memory = malloc(memory_amount);
  assert(result.memory != NULL);
  if (result.memory == NULL) {
    return result;
  }

  AlogRingBuffer ring_buffer =
      alog_ring_buffer_create(configuration.queue_size, elem_size);

  assert(configuration.sink != NULL);
  result.sink = configuration.sink;
  result.ring_buffer = ring_buffer;
  result.max_message_length = configuration.max_message_length;
  result.valid = true;
  return result;
}

static void *alog_logger_flush_continuous(void *hola) {
  puts("Flushing continuously!");
  return NULL;
}

void alog_logger_start_flushing_thread(AlogLogger *logger) {
  assert(logger->flushing_thread == 0);
  pthread_create(&logger->flushing_thread, NULL, alog_logger_flush_continuous,
                 logger);
}

void alog_logger_flush(AlogLogger *logger) {
  while (!alog_ring_buffer_is_empty(logger->ring_buffer)) {
    static Log log;
    assert(alog_ring_buffer_pop(&logger->ring_buffer, &log));
    size_t message_length = log.length;
    printf("message_length: %zu\n", message_length);
    fwrite(log.message, log.length, 1, logger->sink);
  }
}

void alog_logger_free(AlogLogger *logger) {
  assert(logger->valid);
  if (logger->flushing_thread != 0) {
    pthread_cancel(logger->flushing_thread);
    pthread_join(logger->flushing_thread, NULL);
  }
  alog_ring_buffer_free(&logger->ring_buffer);
  free(logger->memory);
  logger->valid = false;
}
