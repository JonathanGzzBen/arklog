#ifndef ARKLOG_H
#define ARKLOG_H

#include "arklog/ring_buffer.h"
#include <string.h>

// Take only filename from __FILE__
#define __FILENAME__                                                           \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// Producer
// Will insert messages into the ring buffer
void alog_log(int level, const char *file, int line, const char *func,
              const char *fmt, ...);

#define ARKLOG_TRACE(...)                                                      \
  alog_log(1, __FILENAME__, __LINE__, __func__, __VA_ARGS__)

/*
#define LOG_DEBUG(...) logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__,
__VA_ARGS__) #define LOG_INFO(...)  logger_log(LOG_LEVEL_INFO, __FILE__,
__LINE__, __func__, __VA_ARGS__) #define LOG_WARN(...)
logger_log(LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, __VA_ARGS__) #define
LOG_ERROR(...) logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__,
__VA_ARGS__) #define LOG_FATAL(...) logger_log(LOG_LEVEL_FATAL, __FILE__,
__LINE__, __func__, __VA_ARGS__)
* */

/**
 * Define intialization functions to configure logger, including memory
 * allocator
 *
 * The flow would involve the 'log' function registering a message into an
 * internal ring buffer.
 *
 * The initialization of the logger should have enough information to allocate
 * all required memory.
 *
 * No further allocations should be needed other than in initialization.
 *
 * When a message is registered using a 'log' function, it is copied into the
 * already owned memory.
 */

typedef struct AlogLoggerConfiguration {
  size_t queue_size;
  size_t max_message_length;
  FILE *sink;
} AlogLoggerConfiguration;

typedef struct AlogLogger {
  AlogRingBuffer *ring_buffer;
  void *memory;
  bool valid;
} AlogLogger;

AlogLogger alog_logger_create(AlogLoggerConfiguration configuration);
void alog_logger_free(AlogLogger *logger);

#endif // ARKLOG_H
