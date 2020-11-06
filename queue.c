/**
 *  @file   queue.c
 *  @brief  Implementation for queue library API
 */

/*************************************************************************
 *                           I N C L U D E S                             *
 *************************************************************************/

#include "queue.h"

/*************************************************************************
 *                            D E F I N E S                              *
 *************************************************************************/

/*************************************************************************
 *                            T Y P E D E S                              *
 *************************************************************************/

/*************************************************************************
 *        P R I V A T E   F U N C T I O N   D E C L A R A T I O N S      *
 *************************************************************************/

/*************************************************************************
 *            P R I V A T E   D A T A   D E C L A R A T I O N S          *
 *************************************************************************/

/*************************************************************************
 *                   P R I V A T E   F U N C T I O N S                   *
 *************************************************************************/

/*************************************************************************
 *                    P U B L I C   F U N C T I O N S                    *
 *************************************************************************/

Queue* Queue_Init(int64_t capacity, int64_t position)
{
  Queue* q = malloc(sizeof(Queue));
  q->head = 0;
  q->tail = 0;
  q->size = 0;
  q->backoff_value = 0;
  q->collision_counter = 0;
  q->position = position;
  q->capacity = capacity;
  q->arr = malloc(sizeof(double) * capacity);

  return q;
}

void Queue_Delete(Queue* q)
{
  if (q == NULL)
  {
    return;
  }

  free(q->arr);
  free(q);
}

double Queue_Enqueue(Queue* q, double val)
{
  if (Queue_IsFull(q))
  {
    return -1;
  }

  q->arr[q->tail] = val;
  q->tail = (q->tail + 1)%q->capacity;
  q->size++;

  return q->size;
}

double Queue_Dequeue(Queue* q)
{
  double retVal = q->arr[q->head];
  q->head = (q->head + 1)%q->capacity;
  q->size--;

  return retVal;
}

bool Queue_IsEmpty(const Queue* q)
{
  if (q->size == 0)
  {
    return true;
  }
  return false;
}

bool Queue_IsFull(const Queue* q)
{
  if (q->size == q->capacity)
  {
    return true;
  }
  return false;
}

double Queue_PeekHead(const Queue* q)
{
    return q->arr[q->head];
}

double Queue_PeekTail(const Queue* q)
{
    return q->arr[q->tail];
}

void Queue_Increment_Collision(Queue* q)
{
  q->collision_counter++;
}

void Queue_Reset_Collision(Queue*q)
{
  q->collision_counter = 0;
}

int Queue_Collision_Count(const Queue* q)
{
  return q->collision_counter;
}

void Queue_update_times(Queue* q, double wait_time)
{
  int pos = q->head;
  do {
    q->arr[pos] = wait_time;
    ++pos;
  } while(q->arr[pos]<wait_time);
}
