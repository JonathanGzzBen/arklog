#ifndef ARKLOG_ATOMIC_RING_BUFFER_H
#define ARKLOG_ATOMIC_RING_BUFFER_H

#include <stdbool.h>
#include <stdatomic.h>
#include <stddef.h>

typedef struct AlogAtomicRingBuffer {
  _Atomic size_t *sequences;
  void *data;
  size_t capacity;
  size_t elem_size;
  _Atomic size_t head;
  _Atomic size_t tail;
} AlogAtomicRingBuffer;

AlogAtomicRingBuffer alog_atomic_ring_buffer_create(size_t elem_count,
                                                    size_t elem_size);
bool alog_atomic_ring_buffer_push(AlogAtomicRingBuffer *ring, void *data);
bool alog_atomic_ring_buffer_pop(AlogAtomicRingBuffer *ring, void *dest);
void alog_atomic_ring_buffer_free(AlogAtomicRingBuffer *ring);

#endif // ARKLOG_ATOMIC_RING_BUFFER_H
