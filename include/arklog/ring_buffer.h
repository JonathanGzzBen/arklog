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
  size_t capacity;
  size_t elem_size;
} AlogRingBuffer;

AlogRingBuffer alog_ring_buffer_create(size_t elem_count, size_t elem_size);
bool alog_ring_buffer_push(AlogRingBuffer *ring, void *data);
bool alog_ring_buffer_pop(AlogRingBuffer *ring, void *dest);

AlogRingBuffer alog_ring_buffer_create(size_t elem_count, size_t elem_size) {
  AlogRingBuffer ring = {.data = (void *)malloc((elem_count + 1) * elem_size),
                         .head = 0,
                         .tail = 0,
                         .capacity = elem_count + 1, // Extra space for wrapping
                         .elem_size = elem_size};
  assert(ring.data != NULL);
  return ring;
}

bool alog_ring_buffer_push(AlogRingBuffer *ring, void *data) {
  assert(ring != NULL);
  assert(data != NULL);

  const size_t new_tail = (ring->tail + 1) % ring->capacity;
  assert(new_tail != ring->head); // Full queue
  void *dest = (char *)ring->data + (ring->tail * ring->elem_size);
  memcpy(dest, data, ring->elem_size);

  ring->tail = new_tail;
  return true;
}

bool alog_ring_buffer_pop(AlogRingBuffer *ring, void *dest) {
  assert(ring != NULL);
  assert(ring->head != ring->tail); // Empty queue

  const size_t new_head = (ring->head + 1) % ring->capacity;
  void *data = (char *)ring->data + (ring->head * ring->elem_size);
  memcpy(dest, data, ring->elem_size);
  ring->head = new_head;
  return true;
}

#endif // ARKLOG_RING_BUFFER_H
