#include "ring_buffer_tests.h"
#include "tests.h"

#include <stdio.h>

#include <arklog/ring_buffer.h>

void test_ring_buffer(void) {
  puts("Testing ring buffer");

  const size_t capacity_for_tests = 3;
  AlogRingBuffer ring_buffer =
      alog_ring_buffer_create(capacity_for_tests, sizeof(int));

  bool test_res = false;
  int test_data = 0;

  for (size_t i = 0; i < capacity_for_tests; i++) {
    if (!(test_res = alog_ring_buffer_push(&ring_buffer, &i)))
      continue;
  }
  test_condition("Can push until full", test_res);
  test_res = alog_ring_buffer_push(&ring_buffer, &test_data);
  test_condition("Push on full returns false", test_res == false);

  for (size_t i = 0; i < capacity_for_tests; i++) {
    int data;
    if (!(test_res = alog_ring_buffer_pop(&ring_buffer, &data)))
      continue;
  }
  test_condition("Can pop until empty", test_res);
  test_res = alog_ring_buffer_pop(&ring_buffer, &test_data);
  test_condition("Pop on empty returns false", test_res == false);

  test_res = alog_ring_buffer_push(&ring_buffer, &test_data);
  test_res = alog_ring_buffer_pop(&ring_buffer, &test_data);
  test_condition("Can push and pop", test_res);
}
