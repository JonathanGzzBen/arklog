#include "lockfree_mpsc_queue_tests.h"
#include "tests.h"

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <arklog/lockfree_mpsc_queue.h>

#define CONCURRENT_ITEM_COUNT 10000

#define MPSC_PRODUCERS          4
#define MPSC_ITEMS_PER_PRODUCER 2500
#define MPSC_TOTAL_ITEMS        (MPSC_PRODUCERS * MPSC_ITEMS_PER_PRODUCER)
#define MPSC_QUEUE_SIZE         64

typedef struct {
  AlogLockfreeMpscQueue *queue;
  int count;
} SpscProducerArgs;

typedef struct {
  AlogLockfreeMpscQueue *queue;
  int count;
  bool fifo_ok;
} SpscConsumerArgs;

typedef struct {
  AlogLockfreeMpscQueue *queue;
  int start_value;
  int count;
} MpscProducerArgs;

typedef struct {
  AlogLockfreeMpscQueue *queue;
  int total;
  bool *received;
  bool no_duplicates;
  bool no_corruption;
} MpscConsumerArgs;

static void *spsc_producer_fn(void *arg);
static void *spsc_consumer_fn(void *arg);
static void *mpsc_producer_fn(void *arg);
static void *mpsc_consumer_fn(void *arg);
static void test_mpsc_integrity(void);

static void *spsc_producer_fn(void *arg) {
  SpscProducerArgs *args = (SpscProducerArgs *)arg;
  for (int i = 0; i < args->count; i++) {
    while (!alog_lockfree_mpsc_queue_push(args->queue, &i))
      ;
  }
  return NULL;
}

static void *spsc_consumer_fn(void *arg) {
  SpscConsumerArgs *args = (SpscConsumerArgs *)arg;
  bool ok = true;
  for (int i = 0; i < args->count; i++) {
    int val;
    while (!alog_lockfree_mpsc_queue_pop(args->queue, &val))
      ;
    if (val != i)
      ok = false;
  }
  args->fifo_ok = ok;
  return NULL;
}

static void *mpsc_producer_fn(void *arg) {
  MpscProducerArgs *args = (MpscProducerArgs *)arg;
  for (int i = 0; i < args->count; i++) {
    int val = args->start_value + i;
    while (!alog_lockfree_mpsc_queue_push(args->queue, &val))
      ;
  }
  return NULL;
}

static void *mpsc_consumer_fn(void *arg) {
  MpscConsumerArgs *args = (MpscConsumerArgs *)arg;
  bool no_dups = true;
  bool no_corrupt = true;
  for (int i = 0; i < args->total; i++) {
    int val;
    while (!alog_lockfree_mpsc_queue_pop(args->queue, &val))
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

static void test_mpsc_integrity(void) {
  AlogLockfreeMpscQueue queue =
      alog_lockfree_mpsc_queue_create(MPSC_QUEUE_SIZE, sizeof(int));

  bool *received = (bool *)calloc((size_t)MPSC_TOTAL_ITEMS, sizeof(bool));
  assert(received != NULL);

  MpscProducerArgs prod_args[MPSC_PRODUCERS];
  pthread_t prod_tids[MPSC_PRODUCERS];

  MpscConsumerArgs cons_args = {.queue = &queue,
                                .total = MPSC_TOTAL_ITEMS,
                                .received = received,
                                .no_duplicates = true,
                                .no_corruption = true};
  pthread_t cons_tid;

  for (int i = 0; i < MPSC_PRODUCERS; i++) {
    prod_args[i].queue = &queue;
    prod_args[i].start_value = i * MPSC_ITEMS_PER_PRODUCER;
    prod_args[i].count = MPSC_ITEMS_PER_PRODUCER;
    pthread_create(&prod_tids[i], NULL, mpsc_producer_fn, &prod_args[i]);
  }
  pthread_create(&cons_tid, NULL, mpsc_consumer_fn, &cons_args);

  for (int i = 0; i < MPSC_PRODUCERS; i++)
    pthread_join(prod_tids[i], NULL);
  pthread_join(cons_tid, NULL);

  bool all_received = true;
  for (int i = 0; i < MPSC_TOTAL_ITEMS; i++) {
    if (!received[i]) {
      all_received = false;
      break;
    }
  }

  test_condition("MPSC: no items lost", all_received);
  test_condition("MPSC: no duplicates", cons_args.no_duplicates);
  test_condition("MPSC: no corruption", cons_args.no_corruption);

  free(received);
  alog_lockfree_mpsc_queue_free(&queue);
}

void test_lockfree_mpsc_queue(void) {
  const size_t capacity_for_tests = 3;
  AlogLockfreeMpscQueue queue =
      alog_lockfree_mpsc_queue_create(capacity_for_tests, sizeof(int));

  bool test_res = false;
  int test_data = 0;

  for (size_t i = 0; i < capacity_for_tests; i++) {
    int val = (int)i;
    if (!(test_res = alog_lockfree_mpsc_queue_push(&queue, &val)))
      continue;
  }
  test_condition("Can push until full", test_res);
  test_res = alog_lockfree_mpsc_queue_push(&queue, &test_data);
  test_condition("Push on full returns false", test_res == false);

  for (size_t i = 0; i < capacity_for_tests; i++) {
    int data;
    if (!(test_res = alog_lockfree_mpsc_queue_pop(&queue, &data)))
      continue;
  }
  test_condition("Can pop until empty", test_res);
  test_res = alog_lockfree_mpsc_queue_pop(&queue, &test_data);
  test_condition("Pop on empty returns false", test_res == false);

  test_res = alog_lockfree_mpsc_queue_push(&queue, &test_data);
  test_res = alog_lockfree_mpsc_queue_pop(&queue, &test_data);
  test_condition("Can push and pop", test_res);

  test_res = alog_lockfree_mpsc_queue_pop(&queue, &test_data);
  test_condition("Queue is empty after draining", test_res == false);

  alog_lockfree_mpsc_queue_free(&queue);
  test_condition(
      "Free zeroes the structure",
      queue.data == NULL && queue.sequences == NULL && queue.capacity == 0 &&
          queue.elem_size == 0 && queue.head == 0 &&
          atomic_load(&queue.tail) == 0);

  AlogLockfreeMpscQueue concurrent_queue =
      alog_lockfree_mpsc_queue_create(8, sizeof(int));

  SpscProducerArgs prod_args = {.queue = &concurrent_queue,
                                .count = CONCURRENT_ITEM_COUNT};
  SpscConsumerArgs cons_args = {.queue = &concurrent_queue,
                                .count = CONCURRENT_ITEM_COUNT,
                                .fifo_ok = false};

  pthread_t prod_tid;
  pthread_t cons_tid;
  pthread_create(&prod_tid, NULL, spsc_producer_fn, &prod_args);
  pthread_create(&cons_tid, NULL, spsc_consumer_fn, &cons_args);
  pthread_join(prod_tid, NULL);
  pthread_join(cons_tid, NULL);

  test_condition("Concurrent SPSC preserves FIFO order", cons_args.fifo_ok);

  alog_lockfree_mpsc_queue_free(&concurrent_queue);

  test_mpsc_integrity();
}
