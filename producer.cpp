#include <iostream>
#include <random>
#include <cstring>
#include <ctime>
#include <unistd.h> //for sleep fn and fork
#include <fcntl.h>  // for O_CREAT
#include <sys/stat.h> // for mode constants
#include <sys/wait.h> // For wait()
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>

using namespace std;

#define MAX_COMMODITY_NAME 11
#define MAX_BOUNDED_BUFFER_SIZE 40

#define SEM_KEY1 0x54321
#define SEM_KEY2 0x54322
#define SEM_KEY3 0x54323

struct shared_buffer{

    char commodities[MAX_BOUNDED_BUFFER_SIZE][MAX_COMMODITY_NAME];
    double prices[MAX_BOUNDED_BUFFER_SIZE]; //prices associated with the commodities
    int in = 0; // points to the next position where the producer will insert a new commodity
    int out = 0; // points to the next position where the consumer will remove a commodity.
    int count = 0; // keeps track of how many items are currently in the buffer.

};

struct Producer_ARGS{

    char commodity_name[MAX_COMMODITY_NAME];
    double commodity_price_mean;
    double commodity_price_stand_dev;
    int sleep_interval_ms;
    int bounded_buffer_size;
};

Producer_ARGS parse_arguments(int argument_count,char *args[])
{
    if(argument_count!=6)
    {
        cerr << "Usage: ./producer <commodity_name> <mean> <stddev> <sleep_interval_ms> <buffer_size>\n";
        exit(1);
    }

    Producer_ARGS array;
    // ensures that no more than MAX_COMMODITY_NAME - 1 characters are written and it automatically places a null terminator at the end of the string
    snprintf(array.commodity_name, MAX_COMMODITY_NAME, "%s", args[1]);
    // after using snprintf as  ensures that the string will not exceed the MAX_COMMODITY_NAME size.

    array.commodity_price_mean = stod(args[2]);
    array.commodity_price_stand_dev = stod(args[3]);
    array.sleep_interval_ms = stoi(args[4]);
    array.bounded_buffer_size = stoi(args[5]);

    if (array.bounded_buffer_size > MAX_BOUNDED_BUFFER_SIZE) {
        cerr << "Error: Buffer size exceeds maximum allowed (" << MAX_BOUNDED_BUFFER_SIZE << ").\n";
        exit(1);
    }

    return array;
 
}

void log_time(const char* message) {
    time_t now = time(nullptr);
    char time_buffer[100];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));
    cerr << "[" << time_buffer << "] " << message << endl;
}



void producer(Producer_ARGS args,struct shared_buffer* buffer)
{
    random_device rd;  // Obtain a random number from hardware
    default_random_engine gen(rd());
    normal_distribution<>price_dist(args.commodity_price_mean,args.commodity_price_stand_dev);

    struct sembuf sem_buf;

    // Creation of semaphore
    int emptyId = semget(SEM_KEY1, 1, 0666 | IPC_CREAT);
    int fullId = semget(SEM_KEY2, 1, 0666 | IPC_CREAT);
    int mutexId = semget(SEM_KEY3, 1, 0666 | IPC_CREAT);

    if (emptyId == -1 || fullId == -1 || mutexId == -1) {
        perror("semget failed");
        exit(1);
    }

    // Initialize semaphores
    semctl(emptyId, 0, SETVAL, args.bounded_buffer_size);
    semctl(fullId, 0, SETVAL, 0);
    semctl(mutexId, 0, SETVAL, 1);


    while(true)
    {
        //Generate a price for the commodity
        double price = abs(price_dist(gen)); // abs for positive prices
        log_time((string(args.commodity_name) + ": generating a new value: " + to_string(price)).c_str());
        
        // Wait for an empty slot
        sem_buf = {0, -1 , 0}; // -1 -> waiting
        semop(emptyId, &sem_buf, 1);

        // Wait to lock the buffer (ensure mutual exclusion)
        log_time((string(args.commodity_name) + ": trying to get mutex on shared buffer").c_str());
        semop(mutexId, &sem_buf, 1);

        //place the price in the buffer
        snprintf(buffer->commodities[buffer->in],MAX_COMMODITY_NAME, "%s",args.commodity_name);
        buffer->prices[buffer->in]=price; // Store the price

        // Update 'in' index (circular buffer)
        buffer->in=((buffer->in)+1) % args.bounded_buffer_size;
        buffer->count++;
        log_time((string(args.commodity_name) + ": placing price " + to_string(price) + " on shared buffer").c_str());

        // Signal that new data is available for consumers
        sem_buf = {0, 1 , 0}; // 1 -> increment the semaphore
        semop(mutexId, &sem_buf, 1);
        semop(fullId, &sem_buf, 1);

        log_time((string(args.commodity_name) + ": sleeping for " + to_string(args.sleep_interval_ms) + " ms").c_str());

        usleep(args.sleep_interval_ms * 1000);

    }
}

int main(int arguments_count, char* arguments[])
{   
    // Parse command-line arguments
    Producer_ARGS args = parse_arguments(arguments_count, arguments);

    // Generate a unique key for shared memory
    key_t key = ftok("file", 65);
    if (key == -1) {
        perror("ftok failed");
        exit(1);
    }
    int shmID = shmget(key, sizeof(struct shared_buffer), 0644 | IPC_CREAT);

    if (shmID == -1) {
        perror("shmget failed");
        exit(1);
    }

    shared_buffer* buffer = (shared_buffer*)shmat(shmID, nullptr, 0);
    if (buffer == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }

    // Initialize shared buffer
    memset(buffer, 0, sizeof(shared_buffer));

    // Start producing
    producer(args, buffer);
    
    // Detach from shared memory
    shmdt(buffer);

    return 0;
}