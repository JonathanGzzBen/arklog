#include "locked_queue_tests.h"
#include "tests.h"

#include <stdio.h>

#include <arklog/locked_queue.h>

void test_locked_queue(void) {
  const size_t capacity_for_tests = 3;
  AlogLockedQueue queue =
      alog_locked_queue_create(capacity_for_tests, sizeof(int));

  bool test_res = false;
  int test_data = 0;

  for (size_t i = 0; i < capacity_for_tests; i++) {
    if (!(test_res = alog_locked_queue_push(&queue, &i)))
      continue;
  }
  test_condition("Can push until full", test_res);
  test_res = alog_locked_queue_push(&queue, &test_data);
  test_condition("Push on full returns false", test_res == false);

  for (size_t i = 0; i < capacity_for_tests; i++) {
    int data;
    if (!(test_res = alog_locked_queue_pop(&queue, &data)))
      continue;
  }
  test_condition("Can pop until empty", test_res);
  test_res = alog_locked_queue_pop(&queue, &test_data);
  test_condition("Pop on empty returns false", test_res == false);

  test_res = alog_locked_queue_push(&queue, &test_data);
  test_res = alog_locked_queue_pop(&queue, &test_data);
  test_condition("Can push and pop", test_res);

  alog_locked_queue_free(&queue);
  test_condition("Free zeroes the structure",
                 queue.data == NULL && queue.capacity == 0 &&
                     queue.elem_size == 0 && queue.head == 0 &&
                     queue.tail == 0);
}
