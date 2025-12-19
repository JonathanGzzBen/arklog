#ifndef ARKLOG_RING_BUFFER_H
#define ARKLOG_RING_BUFFER_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct AlogRingBuffer {
  void *data;
  size_t head;
  size_t tail;
  size_t size;
} AlogRingBuffer;

AlogRingBuffer alog_ring_buffer_create(size_t size);
bool alog_ring_buffer_push(AlogRingBuffer *ring, void *data);
bool alog_ring_buffer_pop(AlogRingBuffer *ring, void *dest);

AlogRingBuffer alog_ring_buffer_create(size_t size) {
  AlogRingBuffer ring;
  ring.data = (void *)malloc((size + 1) * sizeof(void *));
  ring.head = 0;
  ring.tail = 0;
  ring.size = size + 1; // One extra space for wrapping
  return ring;
}

bool alog_ring_buffer_push(AlogRingBuffer *ring, void *data) {
  assert(ring != NULL);
  assert(data != NULL);

  const size_t new_tail = (ring->tail + 1) % ring->size;
  assert(new_tail != ring->head); // Full queue
  void *dest = (char *)ring->data + (ring->tail * sizeof(int));
  memcpy(dest, data, sizeof(int));

  ring->tail = new_tail;
  return true;
}

bool alog_ring_buffer_pop(AlogRingBuffer *ring, void *dest) {
  assert(ring != NULL);
  assert(ring->head != ring->tail); // Empty queue

  const size_t new_head = (ring->head + 1) % ring->size;
  // void *data = ring->data[ring->head];
  void *data = (char *)ring->data + (ring->head * sizeof(int));
  memcpy(dest, data, sizeof(int));
  ring->head = new_head;
  return true;
}

#endif // ARKLOG_RING_BUFFER_H
