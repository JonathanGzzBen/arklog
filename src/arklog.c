#include <stdio.h>

void alog_log(int level, const char *file, int line, const char *func,
              const char *fmt, ...) {
  // Example of desired output
  // [2024-12-09 15:30:45.123456] [Thread-12345] [INFO] [main.c:42] Application
  // started
  // Pending: Timestamp, Thread ID, debug level in text
  printf("[LEVEL %d] [%s:%d] [FUNC: %s] %s\n", level, file, line, func, fmt);
}
