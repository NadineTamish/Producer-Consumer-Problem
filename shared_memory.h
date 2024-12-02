#ifndef shared_memory_h // If SHARED_MEMORY_H is not defined
#define shared_memory_h  // Define SHARED_MEMORY_H

#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h> 
#include <string.h> 
#define MAX_BUFFER_SIZE 40
#define MAX_COMMODITY_NAME 10

// to define a shared memory buffer in producer-consumer system
struct shared_buffer{

    char commodities[MAX_BUFFER_SIZE][MAX_COMMODITY_NAME];
    double prices[MAX_BUFFER_SIZE]; //prices associated with the commodities
    int in; // points to the next position where the producer will insert a new commodity
    int out; // points to the next position where the consumer will remove a commodity.
    int count; // keeps track of how many items are currently in the buffer.

};

// buffer that will point to the allocated shared memory.
int setupSharedMemory(int buffer_size, struct shared_buffer** buffer);

void cleanupSharedMemory(int shmid, struct shared_buffer* buffer);
// The shared memory ID returned by shmget.
// detaches & removes the shared memory segment

int initializeSemaphore(int initial_value);

void semaphoreWait();
void semaphoreSignal();
void cleanupSemaphore();



#endif // End the include guard