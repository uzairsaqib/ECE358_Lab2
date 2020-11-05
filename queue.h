/**
 *  @file   queue.h
 *  @brief  API for queue library
 */

#ifndef __QUEUE_H
#define __QUEUE_H

/*************************************************************************
 *                           I N C L U D E S                             *
 *************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

/*************************************************************************
 *                            D E F I N E S                              *
 *************************************************************************/

/*************************************************************************
 *                            T Y P E D E S                              *
 *************************************************************************/

typedef struct
{
  int64_t head, tail, size, capacity;
  double* arr;
} Queue;

/*************************************************************************
 *          P U B L I C   F U N C T I O N   D E C L A R A T I O N S      *
 *************************************************************************/

/**
 *  @brief  Creates and initializes a queue object
 *  @param  capacity The maximum size of the queue to create
 *  @return Pointer to the created queue
 */
Queue* Queue_Init(int64_t capacity);

/**
 *  @brief  Deletes a queue object
 *  @param  q Pointer to the queue to delete
 */
void Queue_Delete(Queue* q);

/**
 *  @brief  Enqueues a value to a queue
 *  @param  q Queue to operate on
 *  @param  val Value to enqueue
 *  @return Size of queue (-1 if Failed)
 */
double Queue_Enqueue(Queue* q, double val);

/**
 *  @brief  Dequeues a value from a queue
 *  @param  q Queue to operate on
 *  @return Dequeued value (INT16_MIN if dequeue failed)
 */
double Queue_Dequeue(Queue* q);

/**
 *  @brief  Checks whether a queue is empty or not
 *  @param  q Queue to operate on
 *  @return True if empty, False otherwise
 */
bool Queue_IsEmpty(const Queue* q);

/**
 *  @brief  Checks whether a queue is full or not
 *  @param  q Queue to operate on
 *  @return True if full, False otherwise
 */
bool Queue_IsFull(const Queue* q);

/**
 *  @brief  Returns the item at the front of the queue
 *          without dequeueing the item
 *  @return Item at front of queue
 */
double Queue_PeekHead(const Queue *q);

/**
 *  @brief  Returns the item at the tail of the queue
 *          without dequeueing the item
 *  @return Item at back of queue
 */
double Queue_PeekTail(const Queue *q);

#endif /* __QUEUE_H */
