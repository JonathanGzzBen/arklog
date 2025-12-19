#ifndef ARKLOG_RING_BUFFER_H
#define ARKLOG_RING_BUFFER_H

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct AlogRingBuffer {
  void **data;
  size_t head;
  size_t tail;
  size_t size;
} AlogRingBuffer;

AlogRingBuffer alog_ring_buffer_create(size_t size);
void alog_ring_buffer_push(AlogRingBuffer *ring, void *data);
void *alog_ring_buffer_pop(AlogRingBuffer *ring);

AlogRingBuffer alog_ring_buffer_create(size_t size) {
  AlogRingBuffer ring;
  ring.data = (void **)malloc((size + 1) * sizeof(void *));
  ring.head = 0;
  ring.tail = 0;
  ring.size = size + 1; // One extra space for wrapping
  return ring;
}

void alog_ring_buffer_push(AlogRingBuffer *ring, void *data) {
  const size_t new_tail = (ring->tail + 1) % ring->size;
  assert(new_tail != ring->head); // Full queue
  ring->data[ring->tail] = data;
  ring->tail = new_tail;
}

void *alog_ring_buffer_pop(AlogRingBuffer *ring) {
  assert(ring->head != ring->tail); // Empty queue
  const size_t new_head = (ring->head + 1) % ring->size;
  void *data = ring->data[ring->head];
  ring->head = new_head;
  return data;
}

#endif // ARKLOG_RING_BUFFER_H
