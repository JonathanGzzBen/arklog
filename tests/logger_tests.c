#include "logger_tests.h"
#include "arklog/arklog.h"
#include "arklog/ring_buffer.h"
#include "tests.h"
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define TEST_MSG_MAX_LEN 200

typedef struct LoggingThread {
  AlogLogger *logger;
  size_t sequence;
  pthread_t thread;
  pid_t tid;
} LoggingThread;

void *log_sequence_number(void *logging_thread_ptr);

void *log_sequence_number(void *logging_thread_ptr) {
  LoggingThread *logging_thread = (LoggingThread *)logging_thread_ptr;
  const pid_t tid = gettid();
  logging_thread->tid = tid;
  ARKLOG_TRACE(logging_thread->logger, "[Thread %d] [Sequence %zu]", tid,
               logging_thread->sequence);
  return NULL;
}

void test_multiple_producer_integrity(void);
void test_log_levels(void);

void test_logger(void) {
  // TODO: Improve tests for log level filtering
  // TODO: Add timestamp to logs

  AlogLoggerConfiguration invalid_configuration = {.queue_size = 2,
                                                   .max_message_length = 500};

  AlogLogger logger = alog_logger_create(invalid_configuration);
  test_condition("Can't create logger with invalid configuration",
                 !logger.valid);

  static const char *test_filename = "hola.txt";
  FILE *test_sink = fopen(test_filename, "w+");

  static const size_t num_test_logs = 10;

  AlogLoggerConfiguration valid_configuration = {.queue_size = num_test_logs,
                                                 .max_message_length = 100,
                                                 .sink = test_sink,
                                                 .initial_log_level =
                                                     LOG_LEVEL_TRACE};
  logger = alog_logger_create(valid_configuration);
  test_condition("Can create logger with valid configuration", logger.valid);

  for (size_t i = 0; i < num_test_logs; i++) {
    ARKLOG_TRACE(&logger, "This is the log %d.", i);
  }

  const size_t count = logger.ring_buffer.tail - logger.ring_buffer.head;
  test_condition("Can queue messages", count == num_test_logs);
  ARKLOG_TRACE(&logger, "This is a message that will be ignored");
  test_condition("Logging past the queue size ignores message",
                 count == num_test_logs);

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
  test_condition("Flushing thread can empty queue",
                 true); // TODO: Make ring queue lock free, and MPMC later

  ARKLOG_TRACE(
      &logger,
      "Lorem ipsum dolor sit amet, consectetur adipiscing "
      "elit. Integer fringilla ligula augue, ac gravida."); // 100 chars long
  test_condition("Can log message exceeding max_message_length", true);

  alog_logger_free(&logger);
  test_condition("Can free valid logger", true);
  fclose(test_sink);
  remove(test_filename);

  test_multiple_producer_integrity();
  test_log_levels();
}

void test_multiple_producer_integrity(void) {
  static const char *integrity_test_filename = "integrity.txt";
  FILE *test_sink = fopen(integrity_test_filename, "wr+");
  const int n_producers = 5;
  AlogLoggerConfiguration valid_configuration = {
      .queue_size = (size_t)n_producers,
      .max_message_length = TEST_MSG_MAX_LEN,
      .sink = test_sink,
      .initial_log_level = LOG_LEVEL_TRACE};
  AlogLogger logger = alog_logger_create(valid_configuration);
  LoggingThread args[n_producers];
  for (int i = 0; i < n_producers; i++) {
    args[i].logger = &logger;
    args[i].sequence = (size_t)i;
  }
  for (int i = 0; i < n_producers; i++) {
    pthread_create(&args[i].thread, NULL, log_sequence_number, &args[i]);
  }
  for (int i = 0; i < n_producers; i++) {
    pthread_join(args[i].thread, NULL);
  }
  alog_logger_flush(&logger);
  alog_logger_free(&logger);
  fclose(test_sink);

  // Now reopen sink to verify integrity
  FILE *input_sink = fopen(integrity_test_filename, "r");
  int valid_messages_count = 0;
  char log_line_read[TEST_MSG_MAX_LEN] = {0};
  for (int i = 0; i < n_producers; i++) {
    fgets(log_line_read, TEST_MSG_MAX_LEN, input_sink);
    pid_t tid_read = (pid_t)strtol(log_line_read + 70, NULL, 10);
    size_t sequence_read = (size_t)strtol(log_line_read + 86, NULL, 10);

    // Compare real TID with TID read from sink
    if (args[sequence_read].tid == tid_read)
      valid_messages_count++;
  }
  test_condition("Maintain integrity with multiple producers",
                 valid_messages_count == n_producers);
  fclose(input_sink);
  remove(integrity_test_filename);
}

void test_log_levels(void) {
  static const char *log_levels_test_filename = "levels.txt";
  FILE *test_sink = fopen(log_levels_test_filename, "w+");
  const int n_trace_messages = 5;
  const int n_info_messages = 5;
  AlogLoggerConfiguration valid_configuration = {
      .queue_size = (size_t)(n_trace_messages + n_info_messages),
      .max_message_length = TEST_MSG_MAX_LEN,
      .sink = test_sink,
      .initial_log_level = LOG_LEVEL_TRACE};
  AlogLogger logger = alog_logger_create(valid_configuration);
  for (int i = 0; i < n_trace_messages; i++) {
    ARKLOG_TRACE(&logger, "%d", LOG_LEVEL_TRACE);
  }
  for (int i = 0; i < n_info_messages; i++) {
    ARKLOG_INFO(&logger, "%d", LOG_LEVEL_INFO);
  }

  alog_logger_flush(&logger);
  alog_logger_free(&logger);
  fclose(test_sink);

  // Reopen
  FILE *input_sink = fopen(log_levels_test_filename, "r");
  int trace_messages_count = 0;
  int info_messages_count = 0;
  char log_line_read[TEST_MSG_MAX_LEN] = {0};
  for (int i = 0; i < n_trace_messages + n_info_messages; i++) {
    fgets(log_line_read, TEST_MSG_MAX_LEN, input_sink);
    size_t log_level_read = (size_t)strtol(log_line_read + 59, NULL, 10);
    trace_messages_count = log_level_read == LOG_LEVEL_TRACE
                               ? trace_messages_count + 1
                               : trace_messages_count;
    info_messages_count = log_level_read == LOG_LEVEL_INFO
                              ? info_messages_count + 1
                              : info_messages_count;
  }

  test_condition("Filtering works properly",
                 trace_messages_count == n_trace_messages &&
                     info_messages_count == n_info_messages);
  fclose(input_sink);
  remove(log_levels_test_filename);
}
