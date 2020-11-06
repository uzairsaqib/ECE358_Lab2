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
  int64_t position, head, tail, size, capacity, collision_counter;
  double backoff_value;
  double* arr;
} Queue;

/*************************************************************************
 *          P U B L I C   F U N C T I O N   D E C L A R A T I O N S      *
 *************************************************************************/

/**
 *  @brief  Creates and initializes a queue object
 *  @param  capacity The maximum size of the queue to create
 *  @param position The node ID
 *  @return Pointer to the created queue
 */
Queue* Queue_Init(int64_t capacity, int64_t position);

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

/**
 * @brief Incremet the queue collision count
 * @param q Queue to operate on 
 */
void Queue_Increment_Collision(Queue* q);

/**
 * @brief Reset the collision counter of a queue to 0
 * @param q Queue to operate on 
 */
void Queue_Reset_Collision(Queue*q);

/**
 * @brief Get the queue collision count
 * @param q Queue to operate on 
 * @return Amount of collisions detected by the queue
 */
int Queue_Collision_Count(const Queue* q);



/**
 * @brief Update the values of a queue's packets
 * until the packets no longer have a value of wait_time.
 * Assumes that the head MUST be updated
 * @param wait_time the max wait time to check for
 * @param q Queue to operate on
 */
void Queue_update_times(Queue* q, double wait_time);

#endif /* __QUEUE_H */
