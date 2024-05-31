///////////////////// QUEUE IMPLEMENTATION SOURCE FILE README///////////////////////

//This file contains the implementation (`queue.c`) of a thread-safe queue data 
//structure used in a multi-threaded prime number finder program. The queue is 
//implemented as a dynamically allocated array with support for operations like 
//insertion, removal, and destruction.

// *Dynamic memory allocation is used for the queue array.
// *Mutex locks and condition variables are utilized for thread safety.
// *Proper error handling and memory management practices are followed.

//Helped from https://github.com/nealian/cse325_project4/blob/master/sched_impl.c

#include "queue.h"
#include <stdlib.h>

// Initilize the queue
void QueueInitialize(Queue* queue, int max_size) {
    queue->array = (int*)malloc(max_size * sizeof(int)); //Allocate memory
    queue->max_size = max_size;
    queue->current_size = 0;
    queue->front = 0;
    pthread_mutex_init(&queue->mutex, NULL); //Initilize the mutex
    pthread_cond_init(&queue->not_full, NULL); // Initialize the condition variable for not full
    pthread_cond_init(&queue->not_empty, NULL); // Initialize the condition variable for not empty
}

//Insert an element into queue
void QueueInsert(Queue* queue, int value) {
    pthread_mutex_lock(&queue->mutex); // lock
    while (queue->current_size == queue->max_size) {
        pthread_cond_wait(&queue->not_full, &queue->mutex); // Wait condition
    }
    int rear = (queue->front + queue->current_size) % queue->max_size; //rear index
    queue->array[rear] = value;
    queue->current_size++;
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex); //release
}

//Remove the element
int QueueRemove(Queue* queue) {
    pthread_mutex_lock(&queue->mutex); //lock
    while (queue->current_size == 0) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex); // Empty condition
    }
    int value = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->max_size; // Move the front index
    queue->current_size--;
    pthread_cond_signal(&queue->not_full); // Signal that the queue is not full
    pthread_mutex_unlock(&queue->mutex); // release
    return value;
}

// Destroy
void QueueDestroy(Queue* queue) {
    free(queue->array); // Free the memory 
    pthread_mutex_destroy(&queue->mutex); //Mutex
    pthread_cond_destroy(&queue->not_full); //Condition Variable
    pthread_cond_destroy(&queue->not_empty); //Condition Variable
}



