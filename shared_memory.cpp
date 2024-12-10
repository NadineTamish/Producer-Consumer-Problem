
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
using namespace std;

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

// Declare semaphores 
sem_t *empty; //semaphore for the buffer size
sem_t *mutex;//semaphore for mutual exclusion 1/0
sem_t *full;

int initializeSemaphore(int buffer_size) {
    empty = sem_open("/empty", O_CREAT, 0666, buffer_size); // to track the available slots
    if (empty == SEM_FAILED) {
        perror("Semaphore 'empty' initialization failed");
        exit(1);
    }

    mutex=sem_open("/mutex", O_CREAT, 0666, 1); // to track the available slots
    if (mutex == SEM_FAILED) {
        perror("Semaphore 'mutex' initialization failed");
        exit(1);
    }

    full = sem_open("/full", O_CREAT, 0666, 0); // for full slots
    if (full == SEM_FAILED) {
        perror("Semaphore 'full' initialization failed");
        exit(1);
    }
    return 0;
}

void semaphoreWait(sem_t* semaphore) {
    cout<<"entered";
    if (sem_wait(semaphore) == -1) {
        perror("Semaphore wait failed");
        exit(1);
    }
}

void semaphoreSignal(sem_t* semaphore) {
    if (sem_post(semaphore) == -1) {
        perror("Semaphore signal failed");
        exit(1);
    }
}

void cleanupSemaphores() {
    if (sem_close(empty) == -1 || sem_close(mutex) == -1|| sem_close(full) == -1) {
        perror("Semaphore close failed");
        exit(1);
    }
    if (sem_unlink("/empty") == -1 || sem_unlink("/mutex") == -1|| sem_unlink("/full") == -1) {
        perror("Semaphore unlink failed");
        exit(1);
    }
}