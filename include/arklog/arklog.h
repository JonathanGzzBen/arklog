#ifndef ARKLOG_H
#define ARKLOG_H

#include "arklog/lockfree_mpmc_queue.h"
#include "arklog/lockfree_mpsc_queue.h"
#include "arklog/mutex_locked_queue.h"
#include <pthread.h>
#include <string.h>

// Compile-time cap on the per-message buffer (includes log header and newline).
// Effective max is min(AlogLoggerConfiguration.max_message_length, ALOG_MAX_MESSAGE_LENGTH).
// Override at build time with -DARKLOG_MAX_MESSAGE_LENGTH=N passed to CMake.
#ifndef ALOG_MAX_MESSAGE_LENGTH
#define ALOG_MAX_MESSAGE_LENGTH 1024
#endif

// Take only filename from __FILE__
#define __FILENAME__                                                           \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

typedef enum AlogQueueType {
  ALOG_QUEUE_MUTEX_LOCKED,
  ALOG_QUEUE_LOCKFREE_MPMC,
  ALOG_QUEUE_LOCKFREE_MPSC
} AlogQueueType;

typedef enum LogLevel {
  LOG_LEVEL_FATAL,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_WARN,
  LOG_LEVEL_INFO,
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_TRACE
} LogLevel;

typedef struct AlogLoggerConfiguration {
  AlogQueueType queue_type;
  size_t queue_size;
  // Effective maximum is min(max_message_length, ALOG_MAX_MESSAGE_LENGTH).
  // ALOG_MAX_MESSAGE_LENGTH is a compile-time cap (default 1024); override with
  // -DARKLOG_MAX_MESSAGE_LENGTH=N at build time.
  size_t max_message_length;
  FILE *sink;
  LogLevel initial_log_level;
} AlogLoggerConfiguration;

typedef struct AlogLogger {
  AlogQueueType queue_type;
  union {
    AlogMutexLockedQueue mutex_locked;
    AlogLockfreeMpmcQueue lockfree_mpmc;
    AlogLockfreeMpscQueue lockfree_mpsc;
  } queue;
  FILE *sink;
  size_t max_message_length;
  pthread_t flushing_thread;
  bool stop_flag;
  pthread_mutex_t queue_lock; // only used with ALOG_QUEUE_MUTEX_LOCKED
  LogLevel current_log_level;
  bool valid;
} AlogLogger;

typedef struct Log {
  size_t length;
  char *message;
} Log;

// Producer
// Will insert messages into the ring buffer
void alog_log(AlogLogger *logger, int level, const char *file, int line,
              const char *func, const char *fmt, ...);

#define ARKLOG_TRACE(logger, ...)                                              \
  alog_log(logger, LOG_LEVEL_TRACE, __FILENAME__, __LINE__, __func__,          \
           __VA_ARGS__)

#define ARKLOG_INFO(logger, ...)                                               \
  alog_log(logger, LOG_LEVEL_INFO, __FILENAME__, __LINE__, __func__,           \
           __VA_ARGS__)

/*
#define LOG_DEBUG(...) logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__,
__VA_ARGS__) #define LOG_INFO(...)  logger_log(LOG_LEVEL_INFO, __FILE__,
__LINE__, __func__, __VA_ARGS__) #define LOG_WARN(...)
logger_log(LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, __VA_ARGS__) #define
LOG_ERROR(...) logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__,
__VA_ARGS__) #define LOG_FATAL(...) logger_log(LOG_LEVEL_FATAL, __FILE__,
__LINE__, __func__, __VA_ARGS__)
* */

__attribute__((warn_unused_result)) AlogLogger
alog_logger_create(AlogLoggerConfiguration configuration);
void alog_logger_flush(AlogLogger *logger);
void alog_logger_start_flushing_thread(AlogLogger *logger);
void alog_logger_set_log_level(AlogLogger *logger, LogLevel log_level);
void alog_logger_free(AlogLogger *logger);

#endif // ARKLOG_H
