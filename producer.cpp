#include <iostream>
#include <random>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <unistd.h> //for sleep fn and fork
#include <semaphore.h>
#include <fcntl.h>  // for O_CREAT
#include <sys/stat.h> // for mode constants
#include <sys/wait.h> // For wait()
#include <sys/shm.h>
#include <sys/sem.h>
using namespace std;

#define MAX_COMMODITY_NAME 10
#define MAX_BOUNDED_BUFFER_SIZE 40

#define SEM_KEY1 0x54321
#define SEM_KEY2 0x54322
#define SEM_KEY3 0x54323
#define SEM_KEY4 0x54324

struct shared_buffer{

    char commodities[MAX_BOUNDED_BUFFER_SIZE][MAX_COMMODITY_NAME];
    double prices[MAX_BOUNDED_BUFFER_SIZE]; //prices associated with the commodities
    int in; // points to the next position where the producer will insert a new commodity
    int out; // points to the next position where the consumer will remove a commodity.
    int count; // keeps track of how many items are currently in the buffer.

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
        std::cerr << "Usage: ./producer <commodity_name> <mean> <stddev> <sleep_interval_ms> <buffer_size>\n";
        exit(1);
    }

    Producer_ARGS array;
    // ensures that no more than MAX_COMMODITY_NAME - 1 characters are written nd it automatically places a null terminator at the end of the string
    std::snprintf(array.commodity_name, MAX_COMMODITY_NAME, "%s", args[1]);
    // after using snprintf as  ensures that the string will not exceed the MAX_COMMODITY_NAME size.

    array.commodity_price_mean = std::stod(args[2]);
    array.commodity_price_stand_dev = std::stod(args[3]);
    array.sleep_interval_ms = std::stoi(args[4]);
    array.bounded_buffer_size = std::stoi(args[5]);

    // if (strlen(array.commodity_name) > 10) {
    // throw std::invalid_argument("Commodity name must be at most 10 characters.");
    // }

    if (array.bounded_buffer_size > MAX_BOUNDED_BUFFER_SIZE) {
    std::cerr << "Error: Buffer size exceeds maximum allowed (" << MAX_BOUNDED_BUFFER_SIZE << ").\n";
    exit(1);
    }

    return array;
 
}

// Function to log time
// void log_time(const char* message) {
//     struct timespec ts;
//     clock_gettime(CLOCK_REALTIME, &ts);
//     char time_buffer[100];
//     char date_buffer[100];
//     cout<<"hello"<<endl;
//     // Format the time
//     std::strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", std::localtime(&ts.tv_sec));
//     cout<<"hello1"<<endl;
//     // Format the date
//     std::strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", std::localtime(&ts.tv_sec));
//     cout<<"hello2"<<endl;
//     // Print the date, time, and message
//     std::cerr << "[" << date_buffer << " " << time_buffer << "." << ts.tv_nsec / 1000000 << "] " << message << std::endl;
//     std::cout<<"hello3"<<endl;
// }

void log_time(const char* message) {
    std::time_t now = std::time(nullptr);
    char time_buffer[100];
    std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    std::cerr << "[" << time_buffer << "] " << message << std::endl;
}

void sleep_using_clock_gettime(long milliseconds) {
    struct timespec start, current;
    long elapsed_ms = 0;

    // Get the current time
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (elapsed_ms < milliseconds) {
        
        clock_gettime(CLOCK_MONOTONIC, &current);

        elapsed_ms = (current.tv_sec - start.tv_sec) * 1000 +
                     (current.tv_nsec - start.tv_nsec) / 1000000;
    }
}

void producer(Producer_ARGS args,struct shared_buffer* buffer)
{
    cout <<(buffer->in)<<endl;
    cout<<"hi";
    cout<<(buffer->count)<<endl;
    std::random_device rd;  // Obtain a random number from hardware
    std::default_random_engine gen(rd());
    std::normal_distribution<>price_dist(args.commodity_price_mean,args.commodity_price_stand_dev);

    // struct sembuf sem_buf_e;
    // struct sembuf sem_buf_f;
    // struct sembuf sem_buf_m;
    // struct sembuf sem_buf_i;

    struct sembuf sem_buf;

    // int emptyId = semget(SEM_KEY1, 1, 0666);
    // int fullId = semget(SEM_KEY2, 1, 0666);
    // int mutexId = semget(SEM_KEY3, 1, 0666);
    // int indexId = semget(SEM_KEY4, 1, 0666);

    // if (emptyId == -1) {
	// 	// Create new semafor.
	// 	emptyId = semget(SEM_KEY1, 1, 0666 | IPC_CREAT);
    //     semctl(emptyId, 0, SETVAL, args.bounded_buffer_size);
	// }

    //  if (fullId == -1) {
	// 	// Create new semafor.
	// 	fullId = semget(SEM_KEY2, 1, 0666 | IPC_CREAT);
    //     semctl(fullId, 0, SETVAL, 0);
	// }

    // if (mutexId == -1) {
	// 	// Create new semafor.
	// 	mutexId = semget(SEM_KEY3, 1, 0666 | IPC_CREAT);
    //     semctl(mutexId, 0, SETVAL, 1);
    // }

    // if (indexId == -1) {
	// 	// Create new semafor.
	// 	indexId = semget(SEM_KEY4, 1, 0666 | IPC_CREAT);
    //     semctl(indexId, 0, SETVAL, 0);
	// }

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
        std::cout << "Before log_time" << std::endl;
        log_time("Sample message");
        std::cout << "After log_time" << std::endl;
        cout<<"sama";
        //Generate a price for the commodity
        double price=price_dist(gen);
        cout<<"sama1";
        log_time((std::string(args.commodity_name) + ": generating a new value: " + std::to_string(price)).c_str());
        std::cout << "Nadine" << std::endl; 
        // Wait for an empty slot
        // semaphoreWait(empty);
        // wait empty
        sem_buf= {0, -1 , 0};
        semop(emptyId, &sem_buf, 1);

        std::cout << "nadine1" << std::endl;


         // Wait to lock the buffer (ensure mutual exclusion)
        log_time((std::string(args.commodity_name) + ": trying to get mutex on shared buffer").c_str());
        // semaphoreWait(mutex);

        sem_buf = {0, -1 , 0};
        semop(mutexId, &sem_buf, 1);

        // CRITICAL SECTION
        //place the price in the buffer
        snprintf(buffer->commodities[buffer->in],MAX_COMMODITY_NAME, "%s",args.commodity_name);
        buffer->prices[buffer->in]=price; // Store the price
        //cout<< buffer->commodities[buffer->in];
        // Update 'in' index (circular buffer)
        buffer->in=((buffer->in)+1) % args.bounded_buffer_size;
        buffer->count++;


        // Unlock the buffer (release mutex)
        // semaphoreSignal();

        log_time((std::string(args.commodity_name) + ": placing price " + std::to_string(price) + " on shared buffer").c_str());

        // Signal that new data is available for consumers
        // semaphoreSignal(mutex);
        // semaphoreSignal(full);

        sem_buf = {0, 1 , 0};
        semop(mutexId, &sem_buf, 1);

        sem_buf = {0, 1 , 0};
        semop(fullId, &sem_buf, 1);

        log_time((std::string(args.commodity_name) + ": sleeping for " + std::to_string(args.sleep_interval_ms) + " ms").c_str());
        //sleep_using_clock_gettime(args.sleep_interval_ms * 1000); // Sleep in milliseconds
        // std::this_thread::sleep_for(std::chrono::milliseconds(args.sleep_interval_ms)); // Sleep
        usleep(args.sleep_interval_ms * 1000);

    }
}

int main(int arguments_count, char* arguments[])
{
    // struct sembuf sem_buf_e;
    // struct sembuf sem_buf_f;
    // struct sembuf sem_buf_m;
    // struct sembuf sem_buf_i;

    
    // struct shared_buffer* buffer;
    // Setup shared memory
    // int shmid=setupSharedMemory(MAX_BOUNDED_BUFFER_SIZE, &buffer);
    
    // Parse command-line arguments
    Producer_ARGS args = parse_arguments(arguments_count, arguments);
    // initializeSemaphore(args.bounded_buffer_size);
    // Start producing
    // producer(args, buffer);

    // struct shared_buffer *buffer;
    // Generate a unique key for shared memory
    key_t key = ftok("/tmp", 65);
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

    // Initialize shared memory
    memset(buffer, 0, sizeof(shared_buffer));
    buffer->in = 0;
    buffer->out = 0;
    buffer->count = 0;

    producer(args, buffer);
    
    shmdt(buffer);

    return 0;

}