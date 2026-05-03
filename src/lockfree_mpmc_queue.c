#include "arklog/lockfree_mpmc_queue.h"

#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

AlogLockfreeMpmcQueue alog_lockfree_mpmc_queue_create(size_t elem_count,
                                                      size_t elem_size) {
  const size_t capacity = elem_count;

  _Atomic size_t *sequences = malloc(capacity * sizeof(_Atomic size_t));
  assert(sequences != NULL);

  void *data = malloc(capacity * elem_size);
  assert(data != NULL);

  for (size_t i = 0; i < capacity; i++) {
    atomic_init(&sequences[i], i);
  }

  AlogLockfreeMpmcQueue queue;
  queue.sequences = sequences;
  queue.data = data;
  queue.capacity = capacity;
  queue.elem_size = elem_size;
  atomic_init(&queue.head, (size_t)0);
  atomic_init(&queue.tail, (size_t)0);

  return queue;
}

/*
 * Vyukov MPMC bounded queue.
 *
 * Each slot carries a sequence number. A producer claims a slot by CAS-ing
 * the global tail counter; the sequence transitions it writes (pos+1 after
 * write, pos+capacity after read) synchronise with consumers via
 * release/acquire barriers so that memcpy'd data is always visible before the
 * sequence flag that advertises it.
 */

bool alog_lockfree_mpmc_queue_push(AlogLockfreeMpmcQueue *queue, void *data) {
  assert(queue != NULL);
  assert(data != NULL);

  size_t pos = atomic_load_explicit(&queue->tail, memory_order_relaxed);

  for (;;) {
    const size_t slot = pos % queue->capacity;
    const size_t seq =
        atomic_load_explicit(&queue->sequences[slot], memory_order_acquire);
    const ptrdiff_t diff = (ptrdiff_t)seq - (ptrdiff_t)pos;

    if (diff == 0) {
      if (atomic_compare_exchange_weak_explicit(&queue->tail, &pos, pos + 1,
                                                memory_order_relaxed,
                                                memory_order_relaxed)) {
        void *dest = (char *)queue->data + slot * queue->elem_size;
        memcpy(dest, data, queue->elem_size);
        atomic_store_explicit(&queue->sequences[slot], pos + 1,
                              memory_order_release);
        return true;
      }
    } else if (diff < 0) {
      return false; // queue is full
    } else {
      pos = atomic_load_explicit(&queue->tail, memory_order_relaxed);
    }
  }
}

bool alog_lockfree_mpmc_queue_pop(AlogLockfreeMpmcQueue *queue, void *dest) {
  assert(queue != NULL);
  assert(dest != NULL);

  size_t pos = atomic_load_explicit(&queue->head, memory_order_relaxed);

  for (;;) {
    const size_t slot = pos % queue->capacity;
    const size_t seq =
        atomic_load_explicit(&queue->sequences[slot], memory_order_acquire);
    const ptrdiff_t diff = (ptrdiff_t)seq - (ptrdiff_t)(pos + 1);

    if (diff == 0) {
      if (atomic_compare_exchange_weak_explicit(&queue->head, &pos, pos + 1,
                                                memory_order_relaxed,
                                                memory_order_relaxed)) {
        void *src = (char *)queue->data + slot * queue->elem_size;
        memcpy(dest, src, queue->elem_size);
        atomic_store_explicit(&queue->sequences[slot], pos + queue->capacity,
                              memory_order_release);
        return true;
      }
    } else if (diff < 0) {
      return false; // queue is empty
    } else {
      pos = atomic_load_explicit(&queue->head, memory_order_relaxed);
    }
  }
}

void alog_lockfree_mpmc_queue_free(AlogLockfreeMpmcQueue *queue) {
  free(queue->sequences);
  free(queue->data);
  queue->sequences = NULL;
  queue->data = NULL;
  queue->capacity = 0;
  queue->elem_size = 0;
  atomic_store_explicit(&queue->head, (size_t)0, memory_order_relaxed);
  atomic_store_explicit(&queue->tail, (size_t)0, memory_order_relaxed);
}
