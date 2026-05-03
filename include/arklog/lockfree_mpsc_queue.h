#ifndef ARKLOG_LOCKFREE_MPSC_QUEUE_H
#define ARKLOG_LOCKFREE_MPSC_QUEUE_H

#include <stdbool.h>
#include <stdatomic.h>
#include <stddef.h>

typedef struct AlogLockfreeMpscQueue {
  _Atomic size_t *sequences;
  void *data;
  size_t capacity;
  size_t elem_size;
  size_t head;          // plain size_t — only the single consumer touches this
  _Atomic size_t tail;
} AlogLockfreeMpscQueue;

AlogLockfreeMpscQueue alog_lockfree_mpsc_queue_create(size_t elem_count,
                                                      size_t elem_size);
bool alog_lockfree_mpsc_queue_push(AlogLockfreeMpscQueue *queue, void *data);
bool alog_lockfree_mpsc_queue_pop(AlogLockfreeMpscQueue *queue, void *dest);
void alog_lockfree_mpsc_queue_free(AlogLockfreeMpscQueue *queue);

#endif // ARKLOG_LOCKFREE_MPSC_QUEUE_H
