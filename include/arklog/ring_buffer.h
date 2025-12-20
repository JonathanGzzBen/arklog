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

#endif // ARKLOG_RING_BUFFER_H
