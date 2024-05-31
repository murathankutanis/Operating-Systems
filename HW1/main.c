/////////////////////// MAIN SOURCE FILE README/////////////////////////

//This file contains the main source code (`main.c`) for a multi-threaded 
//prime number finder program. The program utilizes a thread-safe queue data 
//structure to generate random numbers and find prime numbers within a specified 
//range using worker threads.

// Command-line Options
// * `-t`: Number of worker threads (default 3)
// * `-q`: Maximum size of the queue (default 5)
// * `-r`: Amount of random numbers (default 10)
// * `-m`: Lower bound of the range of random numbers (default 1)
// * `-n`: Upper bound of the range of random numbers (default 100, maximum 2000)
// * `-g`: Rate of generation time (default 100)

// Dependencies
// * pthread library for multi-threading support.
// * math library for mathematical operations.

#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <stdbool.h>

// Default parameters
#define DEFAULT_WORKER_THREADS 3
#define DEFAULT_QUEUE_SIZE 5
#define DEFAULT_RANDOM_COUNT 10
#define DEFAULT_LOWER_BOUND 1
#define DEFAULT_UPPER_BOUND 100
#define MAX_UPPER_BOUND 2000
#define DEFAULT_GENERATION_RATE 100

Queue queue; 

//Thread Generator
void* GeneratorThread(void* arg) {
    int* params = (int*)arg;
    int random_count = params[0];
    int lower_bound = params[1];
    int upper_bound = params[2];
    int generation_rate = params[3]; // Generation rate
    
    
    unsigned int seed = (unsigned int)time(NULL) + pthread_self(); // Combine time and thread ID as seed
    // Seed assignment obtained from ChatGPT
    
    for (int i = 0; i < random_count; i++) {
        int random_number;
        unsigned int new_seed = seed + i; // Use a different seed for each random number
        random_number = (rand_r(&new_seed) % (upper_bound - lower_bound + 1)) + lower_bound; // random number
        QueueInsert(&queue, random_number);
        
        double rand_num = (double)rand_r(&new_seed) / RAND_MAX; // Random Number with uniform distribution [0 1]
        // Obtained from https://stackoverflow.com/questions/34558230/generating-random-numbers-of-exponential-distribution
        
        double sleep_time = -(1.0 / generation_rate) * log(1 - rand_num); // Exponantial Distribution
        usleep((unsigned int)(sleep_time * 100000));
    }
    return NULL;
}
// Prime Checker
bool IsPrime(int number) {
    if (number <= 1)
        return false;
    for (int i = 2; i * i <= number; i++) {
        if (number % i == 0)
            return false;
    }
    return true;
}

//Worker Thread Function
void* WorkerThread(void* arg) {
    while (true) {
        int number = QueueRemove(&queue); //Remove number from queue
        printf("Thread ID: %ld, Number: %d ", pthread_self(), number);
        if (IsPrime(number)) {
            printf("is a prime number.\n");
        } else {
            printf("is not a prime number. Divisors:");
            for (int i = 1; i <= number; i++) { // Divisor finder
                if (number % i == 0) {
                    printf(" %d", i);
                }
            }
            printf("\n");
        }
    }
    return NULL;
}

//Main Function
int main(int argc, char* argv[]) {
    int worker_threads = DEFAULT_WORKER_THREADS; //Number of workers
    int queue_size = DEFAULT_QUEUE_SIZE; // Max size of the queue
    int random_count = DEFAULT_RANDOM_COUNT; // Number of random numbers
    int lower_bound = DEFAULT_LOWER_BOUND; // Floor of the random numbers
    int upper_bound = DEFAULT_UPPER_BOUND; // Peak of the random numbers
    int generation_rate = DEFAULT_GENERATION_RATE; //Generation rate

    // Parse command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "t:q:r:m:n:g:")) != -1) {
        switch (opt) {
            case 't':
                worker_threads = atoi(optarg);
                break;
            case 'q':
                queue_size = atoi(optarg);
                break;
            case 'r':
                random_count = atoi(optarg);
                break;
            case 'm':
                lower_bound = atoi(optarg);
                break;
            case 'n':
                upper_bound = atoi(optarg);
                if (upper_bound > MAX_UPPER_BOUND) {
                    upper_bound = MAX_UPPER_BOUND;
                }
                break;
            case 'g':
                generation_rate = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-t threads] [-q queue size] [-r random count] [-m lower bound] [-n upper bound] [-g generation rate]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    printf("GENERATION_RATE: %d\n", generation_rate);
    
    //Initialize the queue
    QueueInitialize(&queue, queue_size);

    // Create generator thread
    pthread_t generator_thread;
    int generator_args[4] = {random_count, lower_bound, upper_bound, generation_rate};
    pthread_create(&generator_thread, NULL, GeneratorThread, generator_args);

    
    // Create generator thread
    pthread_t worker_threads_arr[worker_threads];
    for (int i = 0; i < worker_threads; i++) {
        pthread_create(&worker_threads_arr[i], NULL, WorkerThread, NULL);
    }

    //Wait
    pthread_join(generator_thread, NULL);

    // Cancel the threads
    for (int i = 0; i < worker_threads; i++) {
        pthread_cancel(worker_threads_arr[i]);
    }
    
    //Destroy the queue
    QueueDestroy(&queue);

    return 0;
}

