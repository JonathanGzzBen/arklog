#include "arklog/lockfree_mpsc_queue.h"

#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

AlogLockfreeMpscQueue alog_lockfree_mpsc_queue_create(size_t elem_count,
                                                      size_t elem_size) {
  const size_t capacity = elem_count;

  _Atomic size_t *sequences = malloc(capacity * sizeof(_Atomic size_t));
  assert(sequences != NULL);

  void *data = malloc(capacity * elem_size);
  assert(data != NULL);

  for (size_t i = 0; i < capacity; i++) {
    atomic_init(&sequences[i], i);
  }

  AlogLockfreeMpscQueue queue;
  queue.sequences = sequences;
  queue.data = data;
  queue.capacity = capacity;
  queue.elem_size = elem_size;
  queue.head = 0;
  atomic_init(&queue.tail, (size_t)0);

  return queue;
}

/*
 * Vyukov MPSC bounded queue.
 *
 * Push is identical to MPMC — multiple producers CAS on tail.
 * Pop has no CAS: since only one consumer ever calls it, head is a plain
 * size_t and the increment is a direct store rather than a CAS.
 * 
 * This is the current optimal queue for most use cases, since logger
 * only supports one sink, for now.
 */

bool alog_lockfree_mpsc_queue_push(AlogLockfreeMpscQueue *queue, void *data) {
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

bool alog_lockfree_mpsc_queue_pop(AlogLockfreeMpscQueue *queue, void *dest) {
  assert(queue != NULL);
  assert(dest != NULL);

  const size_t pos = queue->head;
  const size_t slot = pos % queue->capacity;
  const size_t seq =
      atomic_load_explicit(&queue->sequences[slot], memory_order_acquire);
  const ptrdiff_t diff = (ptrdiff_t)seq - (ptrdiff_t)(pos + 1);

  if (diff != 0)
    return false; // not ready yet, or empty

  queue->head = pos + 1;
  void *src = (char *)queue->data + slot * queue->elem_size;
  memcpy(dest, src, queue->elem_size);
  atomic_store_explicit(&queue->sequences[slot], pos + queue->capacity,
                        memory_order_release);
  return true;
}

void alog_lockfree_mpsc_queue_free(AlogLockfreeMpscQueue *queue) {
  free(queue->sequences);
  free(queue->data);
  queue->sequences = NULL;
  queue->data = NULL;
  queue->capacity = 0;
  queue->elem_size = 0;
  queue->head = 0;
  atomic_store_explicit(&queue->tail, (size_t)0, memory_order_relaxed);
}
