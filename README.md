# arklog

Multithreaded Logger library written in C

## Features

- Multithreaded logging with bounded ring buffer queues
- Three queue backends selectable at logger init:
  - `ALOG_QUEUE_MUTEX_LOCKED` — mutex-protected, single-threaded or low-contention use
  - `ALOG_QUEUE_LOCKFREE_MPMC` — lock-free Vyukov MPMC, multiple producers and consumers
  - `ALOG_QUEUE_LOCKFREE_MPSC` — lock-free Vyukov MPSC, multiple producers with single consumer (recommended for typical logger use)
- Runtime log level filtering

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
#include "arklog/arklog.h"

int main() {
  FILE *sink = fopen("app.log", "w+");
  AlogLoggerConfiguration configuration = {
      .queue_type = ALOG_QUEUE_LOCKFREE_MPSC,
      .queue_size = 100,
      .max_message_length = 200,
      .sink = sink,
      .initial_log_level = LOG_LEVEL_TRACE};
  AlogLogger logger = alog_logger_create(configuration);
  alog_logger_start_flushing_thread(&logger);
  ARKLOG_TRACE(&logger, "This is the log %d.", 1);
  alog_logger_set_log_level(&logger, LOG_LEVEL_INFO);
  ARKLOG_INFO(&logger, "This is an %s log.", "INFO");
  alog_logger_free(&logger); // Stop incoming messages and wait until queue is emptied
  fclose(sink);
  return 0;
}
```

## API

- `alog_logger_create(AlogLoggerConfiguration configuration)` — Initialize a logger
- `alog_logger_start_flushing_thread(AlogLogger *logger)` — Start a background thread that continuously flushes the message queue
- `alog_logger_flush(AlogLogger *logger)` — Manually flush the message queue to the sink
- `alog_logger_set_log_level(AlogLogger *logger, LogLevel log_level)` — Change log level at runtime
- `alog_logger_free(AlogLogger *logger)` — Stop flushing thread if running, flush remaining messages, and free resources
- `ARKLOG_TRACE(logger, fmt, ...)` — Log a trace message
- `ARKLOG_INFO(logger, fmt, ...)` — Log an info message

## Configuration

```c
typedef struct AlogLoggerConfiguration {
  AlogQueueType queue_type;
  size_t queue_size;
  size_t max_message_length;
  FILE *sink;
  LogLevel initial_log_level;
} AlogLoggerConfiguration;
```

| Field | Description |
|---|---|
| `queue_type` | Queue backend to use. See queue types in Features. Defaults to `ALOG_QUEUE_MUTEX_LOCKED` if unset. |
| `queue_size` | Maximum number of messages the queue will hold. When full, new messages are dropped. |
| `max_message_length` | Maximum characters per log entry, including the header (`[LEVEL TRACE] [file.c:42] [FUNC: foo] `) and trailing newline. |
| `sink` | `FILE*` to write logs to. Must remain open until after `alog_logger_free`. |
| `initial_log_level` | Starting log level. Can be updated at runtime with `alog_logger_set_log_level`. |

Available log levels (from lowest to highest verbosity): `LOG_LEVEL_FATAL`, `LOG_LEVEL_ERROR`, `LOG_LEVEL_WARN`, `LOG_LEVEL_INFO`, `LOG_LEVEL_DEBUG`, `LOG_LEVEL_TRACE`.

## Known Limitations

- When the queue is full, incoming messages are dropped. This is a design choice to prioritize non-blocking behavior.
- `max_message_length` includes the log header, so the usable message body is shorter.

## Future Improvements

See [ROADMAP.md](ROADMAP.md) or issues tagged with `enhancement`.

## License

See [LICENSE.md](LICENSE).
