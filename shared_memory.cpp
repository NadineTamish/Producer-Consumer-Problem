
#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <string.h> 
#include <semaphore.h>
#include <fcntl.h> // For O_CREAT
#include <sys/stat.h> // For mode constants
#include "shared_memory.h"

int setupSharedMemory(int buffer_size, struct shared_buffer** buffer){

    key_t key= ftok("shared_memory",65); //unique key
    //key ensures that multiple processes can use the same shared memory segment by generating a unique identifier
    
    int shmid = shmget(key, sizeof(struct shared_buffer), IPC_CREAT | 0666);
    //shmget --> creates a shared memory segment or retrieves an existing one
    // 0666 (read and write for all users)
    // IPC_CREAT tells the system to create a new shared memory segment if it doesn't exist

    if (shmid ==-1)
    {
        perror("Shared memory creation failed");
        exit(1);
    }

    *buffer =(struct shared_buffer*)shmat(shmid,NULL,0); //pointer to the shared memory segment.
    // shmat attaches the shared memory segment to the process's address space, so it can be accessed like a normal pointer
    // NULL because we are letting the system choose where to attach it, and 0 means no special flags.

    if(*buffer==(void*)-1)
    {
        //if shmat fails,
        perror("Shared memory attachment failed");
        exit(1);
    }

    //memset initializes the shared buffer to 0. 
    //This ensures that the buffer is cleared before we begin using it, and no garbage values exist
    //memset(*buffer, 0, sizeof(struct shared_buffer));
    memset(&(*buffer)->commodities, 0, sizeof((*buffer)->commodities));
    memset(&(*buffer)->prices, 0, sizeof((*buffer)->prices));
    (*buffer)->in = 0; //producer index
    (*buffer)->out = 0; //consumer index
    (*buffer)->count = 0;

    return shmid;

}

void cleanupSharedMemory(int shmid,struct shared_buffer* buffer)
{
    if (shmdt(buffer) == -1) {
        perror("Shared memory detachment failed");
        exit(1);
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("Shared memory removal failed");
        exit(1);
    }
}

// Declare a semaphore variable 
sem_t *sem;

int initializeSemaphore(int initial_value) {
    sem = sem_open("/semaphore", O_CREAT, 0666, initial_value);
    if (sem == SEM_FAILED) {
        perror("Semaphore initialization failed");
        exit(1);
    }
    return 0;
}

void semaphoreSignal() {
    //increments the semaphore value.
    if (sem_post(sem) == -1) {
        perror("Semaphore signal failed");
        exit(1);
    }
}

void semaphoreWait() {
    // decrements the semaphore value and blocks if the value is 0
    if (sem_wait(sem) == -1) {
        perror("Semaphore wait failed");
        exit(1);
    }
}

void cleanupSemaphore() {
    // closes the semaphore
    if (sem_close(sem) == -1) {
        perror("Semaphore close failed");
        exit(1);
    }
    // unlinks the semaphore (removes it from the system)
    if (sem_unlink("/semaphore") == -1) {
        perror("Semaphore unlink failed");
        exit(1);
    }
}
