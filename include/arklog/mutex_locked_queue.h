#ifndef ARKLOG_MUTEX_LOCKED_QUEUE_H
#define ARKLOG_MUTEX_LOCKED_QUEUE_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct AlogMutexLockedQueue {
  void *data;
  size_t head;
  size_t tail;
  size_t capacity;
  size_t elem_size;
} AlogMutexLockedQueue;

AlogMutexLockedQueue alog_mutex_locked_queue_create(size_t elem_count,
                                                    size_t elem_size);
bool alog_mutex_locked_queue_push(AlogMutexLockedQueue *queue, void *data);
bool alog_mutex_locked_queue_pop(AlogMutexLockedQueue *queue, void *dest);
void alog_mutex_locked_queue_free(AlogMutexLockedQueue *queue);

#endif // ARKLOG_MUTEX_LOCKED_QUEUE_H
