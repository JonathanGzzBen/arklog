#include "arklog/atomic_ring_buffer.h"

#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

AlogAtomicRingBuffer alog_atomic_ring_buffer_create(size_t elem_count,
                                                    size_t elem_size) {
  const size_t capacity = elem_count;

  _Atomic size_t *sequences = malloc(capacity * sizeof(_Atomic size_t));
  assert(sequences != NULL);

  void *data = malloc(capacity * elem_size);
  assert(data != NULL);

  for (size_t i = 0; i < capacity; i++) {
    atomic_init(&sequences[i], i);
  }

  AlogAtomicRingBuffer ring;
  ring.sequences = sequences;
  ring.data = data;
  ring.capacity = capacity;
  ring.elem_size = elem_size;
  atomic_init(&ring.head, (size_t)0);
  atomic_init(&ring.tail, (size_t)0);

  return ring;
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

bool alog_atomic_ring_buffer_push(AlogAtomicRingBuffer *ring, void *data) {
  assert(ring != NULL);
  assert(data != NULL);

  size_t pos = atomic_load_explicit(&ring->tail, memory_order_relaxed);

  for (;;) {
    const size_t slot = pos % ring->capacity;
    const size_t seq =
        atomic_load_explicit(&ring->sequences[slot], memory_order_acquire);
    const ptrdiff_t diff = (ptrdiff_t)seq - (ptrdiff_t)pos;

    if (diff == 0) {
      if (atomic_compare_exchange_weak_explicit(&ring->tail, &pos, pos + 1,
                                                memory_order_relaxed,
                                                memory_order_relaxed)) {
        void *dest = (char *)ring->data + slot * ring->elem_size;
        memcpy(dest, data, ring->elem_size);
        atomic_store_explicit(&ring->sequences[slot], pos + 1,
                              memory_order_release);
        return true;
      }
    } else if (diff < 0) {
      return false; // queue is full
    } else {
      pos = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    }
  }
}

bool alog_atomic_ring_buffer_pop(AlogAtomicRingBuffer *ring, void *dest) {
  assert(ring != NULL);
  assert(dest != NULL);

  size_t pos = atomic_load_explicit(&ring->head, memory_order_relaxed);

  for (;;) {
    const size_t slot = pos % ring->capacity;
    const size_t seq =
        atomic_load_explicit(&ring->sequences[slot], memory_order_acquire);
    const ptrdiff_t diff = (ptrdiff_t)seq - (ptrdiff_t)(pos + 1);

    if (diff == 0) {
      if (atomic_compare_exchange_weak_explicit(&ring->head, &pos, pos + 1,
                                                memory_order_relaxed,
                                                memory_order_relaxed)) {
        void *src = (char *)ring->data + slot * ring->elem_size;
        memcpy(dest, src, ring->elem_size);
        atomic_store_explicit(&ring->sequences[slot], pos + ring->capacity,
                              memory_order_release);
        return true;
      }
    } else if (diff < 0) {
      return false; // queue is empty
    } else {
      pos = atomic_load_explicit(&ring->head, memory_order_relaxed);
    }
  }
}

void alog_atomic_ring_buffer_free(AlogAtomicRingBuffer *ring) {
  free(ring->sequences);
  free(ring->data);
  ring->sequences = NULL;
  ring->data = NULL;
  ring->capacity = 0;
  ring->elem_size = 0;
  atomic_store_explicit(&ring->head, (size_t)0, memory_order_relaxed);
  atomic_store_explicit(&ring->tail, (size_t)0, memory_order_relaxed);
}
