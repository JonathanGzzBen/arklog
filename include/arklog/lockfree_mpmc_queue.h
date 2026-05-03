#ifndef ARKLOG_LOCKFREE_MPMC_QUEUE_H
#define ARKLOG_LOCKFREE_MPMC_QUEUE_H

#include <stdbool.h>
#include <stdatomic.h>
#include <stddef.h>

typedef struct AlogLockfreeMpmcQueue {
  _Atomic size_t *sequences;
  void *data;
  size_t capacity;
  size_t elem_size;
  _Atomic size_t head;
  _Atomic size_t tail;
} AlogLockfreeMpmcQueue;

AlogLockfreeMpmcQueue alog_lockfree_mpmc_queue_create(size_t elem_count,
                                                      size_t elem_size);
bool alog_lockfree_mpmc_queue_push(AlogLockfreeMpmcQueue *queue, void *data);
bool alog_lockfree_mpmc_queue_pop(AlogLockfreeMpmcQueue *queue, void *dest);
void alog_lockfree_mpmc_queue_free(AlogLockfreeMpmcQueue *queue);

#endif // ARKLOG_LOCKFREE_MPMC_QUEUE_H
