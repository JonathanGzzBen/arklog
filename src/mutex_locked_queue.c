#include "arklog/mutex_locked_queue.h"

AlogMutexLockedQueue alog_mutex_locked_queue_create(size_t elem_count,
                                                    size_t elem_size) {
  AlogMutexLockedQueue queue = {
      .data = (void *)malloc((elem_count + 1) * elem_size),
      .head = 0,
      .tail = 0,
      .capacity = elem_count + 1,
      .elem_size = elem_size};
  assert(queue.data != NULL);
  return queue;
}

bool alog_mutex_locked_queue_push(AlogMutexLockedQueue *queue, void *data) {
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

bool alog_mutex_locked_queue_pop(AlogMutexLockedQueue *queue, void *dest) {
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

void alog_mutex_locked_queue_free(AlogMutexLockedQueue *queue) {
  free(queue->data);
  memset(queue, 0, sizeof(AlogMutexLockedQueue));
}
