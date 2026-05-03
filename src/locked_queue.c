#include "arklog/locked_queue.h"

AlogLockedQueue alog_locked_queue_create(size_t elem_count, size_t elem_size) {
  AlogLockedQueue queue = {
      .data = (void *)malloc((elem_count + 1) * elem_size),
      .head = 0,
      .tail = 0,
      .capacity = elem_count + 1,
      .elem_size = elem_size};
  assert(queue.data != NULL);
  return queue;
}

bool alog_locked_queue_push(AlogLockedQueue *queue, void *data) {
  assert(queue != NULL);
  assert(data != NULL);

  const size_t new_tail = (queue->tail + 1) % queue->capacity;
  if (new_tail == queue->head)
    return false;
  void *dest = (char *)queue->data + (queue->tail * queue->elem_size);
  memcpy(dest, data, queue->elem_size);

  queue->tail = new_tail;
  return true;
}

bool alog_locked_queue_pop(AlogLockedQueue *queue, void *dest) {
  assert(queue != NULL);
  assert(dest != NULL);
  if (queue->head == queue->tail)
    return false;

  const size_t new_head = (queue->head + 1) % queue->capacity;
  void *data = (char *)queue->data + (queue->head * queue->elem_size);
  memcpy(dest, data, queue->elem_size);
  queue->head = new_head;
  return true;
}

void alog_locked_queue_free(AlogLockedQueue *queue) {
  free(queue->data);
  memset(queue, 0, sizeof(AlogLockedQueue));
}
