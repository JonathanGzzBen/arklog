#ifndef ARKLOG_LOCKED_QUEUE_H
#define ARKLOG_LOCKED_QUEUE_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct AlogLockedQueue {
  void *data;
  size_t head;
  size_t tail;
  size_t capacity;
  size_t elem_size;
} AlogLockedQueue;

AlogLockedQueue alog_locked_queue_create(size_t elem_count, size_t elem_size);
bool alog_locked_queue_push(AlogLockedQueue *queue, void *data);
bool alog_locked_queue_pop(AlogLockedQueue *queue, void *dest);
void alog_locked_queue_free(AlogLockedQueue *queue);

#endif // ARKLOG_LOCKED_QUEUE_H
