# arklog

Multitheaded Logger library written in C

## Features

- Multithreaded logging with ring buffers
- Can filter based on log level which can be updated on runtime.

## Requirements

- C11 or later
- POSIX threads (pthread)
- C Extensions (nanosleep())

## Building
```bash
cmake -B build -S .
cmake --build build -j
```

## Build and run tests
```bash
cmake -B build -S .
cmake --build build -j
./build/arklog_test
```

## Usage
```c
#include "arklog.h"

int main() {
  FILE *test_sink = fopen("test_sink.txt", "w+");
  AlogLoggerConfiguration configuration = {.queue_size = 100,
                                                 .max_message_length = 100,
                                                 .sink = test_sink,
                                                 .initial_log_level =
                                                     LOG_LEVEL_TRACE};
  AlogLogger logger = alog_logger_create(configuration);
  alog_logger_start_flushing_thread(&logger);
  ARKLOG_TRACE(&logger, "This is the log %d.", 1);
  alog_logger_set_log_level(&logger, LOG_LEVEL_INFO);
  ARKLOG_INFO(&logger, "This is an %s log.", "INFO");
  alog_logger_free(&logger); // Stop incoming messages and wait until queue is emptied
  fclose(test_sink);
  return 0;
}
```

## API

Brief overview of main functions:

- `alog_logger_create(AlogLoggerConfiguration configuration)` - Initialize a logger
- `alog_logger_start_flushing_thread(AlogLogger *logger)` - Start a thread that will continuously flush message queue
- `alog_logger_flush(AlogLogger *logger)` - Manually flush message queue
- `ARKLOG_INFO(fmt, ...)` - Log info message
- `ARKLOG_TRACE(fmt, ...)` - Log trace message
- `alog_logger_set_log_level(AlogLogger *logger, LogLevel log_level)` - Set log level
- `alog_logger_free(AlogLogger *logger)` - Stop flushing thread if running, flush queue and free resources

## Configuration

```c
typedef struct AlogLoggerConfiguration {
  size_t queue_size;
  size_t max_message_length;
  FILE *sink;
  LogLevel initial_log_level;
} AlogLoggerConfiguration;
```

- `queue_size` is the max amount of messages that the queue will hold
- `max_message_length` is the max amount of chars that a log message will have. Includes log header and line break.
- `sink` is a `FILE*` to which logs will get send to. You have to manually close it after calling `alog_logger_free(...)`
- `initial_log_level` is only an initial value. It can be later be updated with `alog_logger_set_log_level(...)`


## Known Limitations

- When queue is full, incoming messages will get ignored. This is a design choice to prioritize non-blocking behavior.
- `max_message_length` in configuration structure also includes the log header `[LEVEL TRACE] [logger_tests.c:58] [FUNC: te...`

## Future Improvements

See [ROADMAP.md](ROADMAP.md) or issues tagged with `enhancement`.

## License

See [LICENSE.md](LICENSE).

