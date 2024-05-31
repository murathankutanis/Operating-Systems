///////////////////// QUEUE IMPLEMENTATION FILE README///////////////////////

//This file contains the header (`queue.h`) for a thread-safe queue 
//implementation used in a multi-threaded prime number finder program. The 
//queue data structure is implemented using dynamic memory allocation and 
//provides functions for initializing, inserting, removing, and destroying 
//elements.

// FUNCTIONALITY 
// *`QueueInitialize`: Initialize the queue data structure.
// *`QueueInsert`: Insert an element into the queue.
// *`QueueRemove`: Remove an element from the queue.
// *`QueueDestroy`: Destroy the queue data structure and release allocated memory.

//The queue implementation in `queue.h` is designed to be thread-safe using mutex 
//locks and condition variables to ensure proper synchronization in multi-threaded 
//environments.


#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

// Structure
typedef struct {
    int* array;
    int max_size;
    int current_size;
    int front;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} Queue;


// Function prototypes
void QueueInitialize(Queue* queue, int max_size); // Initilize the necessary fields of the queue
void QueueInsert(Queue* queue, int value); // Insert an integer to the queue
int QueueRemove(Queue* queue); // Remove an integer from the queue
void QueueDestroy(Queue* queue); // Destroy the necessary fileds of the queue

#endif /* QUEUE_H */

