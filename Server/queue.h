#ifndef _QUEUE_INCLUDED
#define _QUEUE_INCLUDED

#include <pthread.h>

typedef struct queue Queue;
typedef void *Item;

/*
 * create a Queue that holds Items
 * returns NULL if the create call failed (malloc failure)
 */
Queue *q_create(void);
/*
 * add Item to the Queue; 
 * return 1/0 if successful/not-successful
 */
int q_add(Queue *q, Item i);
/*
 * remove next Item from queue; returns NULL if queue is empty
 */
Item q_remove(Queue *q);

/*
 * returns if a Queue is empty
 */
int isEmpty(Queue *q);

/*
 * releases the memory for the queue
 */
void q_destroy(Queue *q);

/*
 * returns the mutex of the queue
 */
pthread_mutex_t* getMutex(Queue *q);

/*
 * returns the cond of the queue
 */
pthread_cond_t* getCond(Queue *q);

#endif /* _QUEUE_INCLUDED */
