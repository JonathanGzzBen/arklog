#include "logger_tests.h"
#include "arklog/arklog.h"
#include "tests.h"
#include <stdio.h>

void test_logger(void) {
  AlogLoggerConfiguration invalid_configuration = {.queue_size = 2,
                                                   .max_message_length = 10};

  AlogLogger logger = alog_logger_create(invalid_configuration);
  test_condition("Can't create logger with invalid configuration",
                 !logger.valid);

  static const char *test_filename = "hola.txt";
  FILE *test_sink = fopen(test_filename, "w+");

  AlogLoggerConfiguration valid_configuration = {
      .queue_size = 2, .max_message_length = 10, .sink = test_sink};
  logger = alog_logger_create(valid_configuration);
  test_condition("Can create logger with valid configuration", logger.valid);

  fclose(test_sink);
  remove(test_filename);
}
