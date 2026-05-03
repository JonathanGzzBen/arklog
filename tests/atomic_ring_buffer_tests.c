#include "atomic_ring_buffer_tests.h"
#include "tests.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>

#include <arklog/atomic_ring_buffer.h>

#define CONCURRENT_ITEM_COUNT 10000

typedef struct {
  AlogAtomicRingBuffer *ring;
  int count;
} ProducerArgs;

typedef struct {
  AlogAtomicRingBuffer *ring;
  int count;
  bool fifo_ok;
} ConsumerArgs;

static void *producer_fn(void *arg);
static void *consumer_fn(void *arg);

static void *producer_fn(void *arg) {
  ProducerArgs *args = (ProducerArgs *)arg;
  for (int i = 0; i < args->count; i++) {
    while (!alog_atomic_ring_buffer_push(args->ring, &i))
      ;
  }
  return NULL;
}

static void *consumer_fn(void *arg) {
  ConsumerArgs *args = (ConsumerArgs *)arg;
  bool ok = true;
  for (int i = 0; i < args->count; i++) {
    int val;
    while (!alog_atomic_ring_buffer_pop(args->ring, &val))
      ;
    if (val != i)
      ok = false;
  }
  args->fifo_ok = ok;
  return NULL;
}

void test_atomic_ring_buffer(void) {
  const size_t capacity_for_tests = 3;
  AlogAtomicRingBuffer ring =
      alog_atomic_ring_buffer_create(capacity_for_tests, sizeof(int));

  bool test_res = false;
  int test_data = 0;

  for (size_t i = 0; i < capacity_for_tests; i++) {
    int val = (int)i;
    if (!(test_res = alog_atomic_ring_buffer_push(&ring, &val)))
      continue;
  }
  test_condition("Can push until full", test_res);
  test_res = alog_atomic_ring_buffer_push(&ring, &test_data);
  test_condition("Push on full returns false", test_res == false);

  for (size_t i = 0; i < capacity_for_tests; i++) {
    int data;
    if (!(test_res = alog_atomic_ring_buffer_pop(&ring, &data)))
      continue;
  }
  test_condition("Can pop until empty", test_res);
  test_res = alog_atomic_ring_buffer_pop(&ring, &test_data);
  test_condition("Pop on empty returns false", test_res == false);

  test_res = alog_atomic_ring_buffer_push(&ring, &test_data);
  test_res = alog_atomic_ring_buffer_pop(&ring, &test_data);
  test_condition("Can push and pop", test_res);

  test_res = alog_atomic_ring_buffer_pop(&ring, &test_data);
  test_condition("Queue is empty after draining", test_res == false);

  alog_atomic_ring_buffer_free(&ring);
  test_condition(
      "Free zeroes the structure",
      ring.data == NULL && ring.sequences == NULL && ring.capacity == 0 &&
          ring.elem_size == 0 && atomic_load(&ring.head) == 0 &&
          atomic_load(&ring.tail) == 0);

  AlogAtomicRingBuffer concurrent_ring =
      alog_atomic_ring_buffer_create(8, sizeof(int));

  ProducerArgs prod_args = {.ring = &concurrent_ring,
                            .count = CONCURRENT_ITEM_COUNT};
  ConsumerArgs cons_args = {
      .ring = &concurrent_ring, .count = CONCURRENT_ITEM_COUNT, .fifo_ok = false};

  pthread_t prod_tid;
  pthread_t cons_tid;
  pthread_create(&prod_tid, NULL, producer_fn, &prod_args);
  pthread_create(&cons_tid, NULL, consumer_fn, &cons_args);
  pthread_join(prod_tid, NULL);
  pthread_join(cons_tid, NULL);

  test_condition("Concurrent SPSC preserves FIFO order", cons_args.fifo_ok);

  alog_atomic_ring_buffer_free(&concurrent_ring);
}
