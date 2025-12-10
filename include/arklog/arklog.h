#ifndef ARKLOG_H
#define ARKLOG_H

#include <string.h>

// Take only filename from __FILE__
#define __FILENAME__                                                           \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

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
#endif // ARKLOG_H
