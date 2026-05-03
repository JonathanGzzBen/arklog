#ifndef ARKLOG_LOCKFREE_QUEUE_H
#define ARKLOG_LOCKFREE_QUEUE_H

#include <stdbool.h>
#include <stdatomic.h>
#include <stddef.h>

typedef struct AlogLockfreeQueue {
  _Atomic size_t *sequences;
  void *data;
  size_t capacity;
  size_t elem_size;
  _Atomic size_t head;
  _Atomic size_t tail;
} AlogLockfreeQueue;

AlogLockfreeQueue alog_lockfree_queue_create(size_t elem_count,
                                             size_t elem_size);
bool alog_lockfree_queue_push(AlogLockfreeQueue *queue, void *data);
bool alog_lockfree_queue_pop(AlogLockfreeQueue *queue, void *dest);
void alog_lockfree_queue_free(AlogLockfreeQueue *queue);

#endif // ARKLOG_LOCKFREE_QUEUE_H
