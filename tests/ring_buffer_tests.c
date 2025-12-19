#include "ring_buffer_tests.h"

#include <stdio.h>

#include <arklog/ring_buffer.h>

void test_ring_buffer(void) {
  puts("Testing ring buffer");

  AlogRingBuffer ring_buffer = alog_ring_buffer_create(5);
  for (int i = 0; i < 5; i++) {
    printf("Push %d\n", i + 1);
    alog_ring_buffer_push(&ring_buffer, &i);
  }
  for (int i = 0; i < 5; i++) {
    printf("Popping %d\n", i);
    // int *data = (int *)alog_ring_buffer_pop(&ring_buffer);
    int data;
    alog_ring_buffer_pop(&ring_buffer, &data);
    printf("Element at index %d: %d\n", i, data);
  }
}
