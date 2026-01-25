#include "logger_tests.h"
#include "arklog/arklog.h"
#include "arklog/ring_buffer.h"
#include "tests.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>

void test_logger(void) {
  AlogLoggerConfiguration invalid_configuration = {.queue_size = 2,
                                                   .max_message_length = 500};

  AlogLogger logger = alog_logger_create(invalid_configuration);
  test_condition("Can't create logger with invalid configuration",
                 !logger.valid);

  static const char *test_filename = "hola.txt";
  FILE *test_sink = fopen(test_filename, "w+");

  AlogLoggerConfiguration valid_configuration = {
      .queue_size = 2, .max_message_length = 500, .sink = test_sink};
  logger = alog_logger_create(valid_configuration);
  test_condition("Can create logger with valid configuration", logger.valid);

  ARKLOG_TRACE(&logger, "Hola primero");
  ARKLOG_TRACE(&logger, "Hola desde PID %d", getpid());

  size_t count = logger.ring_buffer.tail - logger.ring_buffer.head;
  test_condition("Can queue two messages", count == 2);

  alog_logger_start_flushing_thread(&logger);
  test_condition("Can start flushing thread", true);
  while (true) {
    pthread_mutex_lock(&logger.queue_lock);
    bool is_empty = alog_ring_buffer_is_empty(logger.ring_buffer);
    pthread_mutex_unlock(&logger.queue_lock);
    if (is_empty)
      break;
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000L; // 100ms
    nanosleep(&ts, NULL);
  }
  test_condition("Flushing thread can empty queue", true);

  alog_logger_free(&logger);
  test_condition("Can free valid logger", true);
  fclose(test_sink);
  remove(test_filename);
}
