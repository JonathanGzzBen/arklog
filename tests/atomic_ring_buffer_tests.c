#include "atomic_ring_buffer_tests.h"
#include "tests.h"

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <arklog/atomic_ring_buffer.h>

#define CONCURRENT_ITEM_COUNT 10000

#define MPMC_PRODUCERS          4
#define MPMC_ITEMS_PER_PRODUCER 2500
#define MPMC_TOTAL_ITEMS        (MPMC_PRODUCERS * MPMC_ITEMS_PER_PRODUCER)
#define MPMC_QUEUE_SIZE         64

typedef struct {
  AlogAtomicRingBuffer *ring;
  int count;
} ProducerArgs;

typedef struct {
  AlogAtomicRingBuffer *ring;
  int count;
  bool fifo_ok;
} ConsumerArgs;

typedef struct {
  AlogAtomicRingBuffer *ring;
  int start_value;
  int count;
} MPMCProducerArgs;

typedef struct {
  AlogAtomicRingBuffer *ring;
  int total;
  bool *received;
  bool no_duplicates;
  bool no_corruption;
} MPMCConsumerArgs;

static void *producer_fn(void *arg);
static void *consumer_fn(void *arg);
static void *mpmc_producer_fn(void *arg);
static void *mpmc_consumer_fn(void *arg);
static void test_mpmc_integrity(void);

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

static void *mpmc_producer_fn(void *arg) {
  MPMCProducerArgs *args = (MPMCProducerArgs *)arg;
  for (int i = 0; i < args->count; i++) {
    int val = args->start_value + i;
    while (!alog_atomic_ring_buffer_push(args->ring, &val))
      ;
  }
  return NULL;
}

static void *mpmc_consumer_fn(void *arg) {
  MPMCConsumerArgs *args = (MPMCConsumerArgs *)arg;
  bool no_dups = true;
  bool no_corrupt = true;
  for (int i = 0; i < args->total; i++) {
    int val;
    while (!alog_atomic_ring_buffer_pop(args->ring, &val))
      ;
    if (val < 0 || val >= args->total) {
      no_corrupt = false;
    } else if (args->received[val]) {
      no_dups = false;
    } else {
      args->received[val] = true;
    }
  }
  args->no_duplicates = no_dups;
  args->no_corruption = no_corrupt;
  return NULL;
}

static void test_mpmc_integrity(void) {
  AlogAtomicRingBuffer ring =
      alog_atomic_ring_buffer_create(MPMC_QUEUE_SIZE, sizeof(int));

  bool *received = (bool *)calloc((size_t)MPMC_TOTAL_ITEMS, sizeof(bool));
  assert(received != NULL);

  MPMCProducerArgs prod_args[MPMC_PRODUCERS];
  pthread_t prod_tids[MPMC_PRODUCERS];

  MPMCConsumerArgs cons_args = {.ring = &ring,
                                .total = MPMC_TOTAL_ITEMS,
                                .received = received,
                                .no_duplicates = true,
                                .no_corruption = true};
  pthread_t cons_tid;

  for (int i = 0; i < MPMC_PRODUCERS; i++) {
    prod_args[i].ring = &ring;
    prod_args[i].start_value = i * MPMC_ITEMS_PER_PRODUCER;
    prod_args[i].count = MPMC_ITEMS_PER_PRODUCER;
    pthread_create(&prod_tids[i], NULL, mpmc_producer_fn, &prod_args[i]);
  }
  pthread_create(&cons_tid, NULL, mpmc_consumer_fn, &cons_args);

  for (int i = 0; i < MPMC_PRODUCERS; i++)
    pthread_join(prod_tids[i], NULL);
  pthread_join(cons_tid, NULL);

  bool all_received = true;
  for (int i = 0; i < MPMC_TOTAL_ITEMS; i++) {
    if (!received[i]) {
      all_received = false;
      break;
    }
  }

  test_condition("MPMC: no items lost", all_received);
  test_condition("MPMC: no duplicates", cons_args.no_duplicates);
  test_condition("MPMC: no corruption", cons_args.no_corruption);

  free(received);
  alog_atomic_ring_buffer_free(&ring);
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

  test_mpmc_integrity();
}
