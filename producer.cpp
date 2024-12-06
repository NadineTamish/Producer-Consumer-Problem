#include <iostream>
#include <random>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <unistd.h> //for sleep fn
#include <semaphore.h>
#include <fcntl.h>  // for O_CREAT
#include <sys/stat.h> // for mode constants
#include "shared_memory.h"  // Assuming shared memory and buffer are defined her
#include <thread>

#define MAX_COMMODITY_NAME 10
#define MAX_BOUNDED_BUFFER_SIZE 40


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
void log_time(const char* message) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    char time_buffer[100];
    char date_buffer[100];

    // Format the time
    std::strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", std::localtime(&ts.tv_sec));
    
    // Format the date
    std::strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", std::localtime(&ts.tv_sec));

    // Print the date, time, and message
    std::cerr << "[" << date_buffer << " " << time_buffer << "." << ts.tv_nsec / 1000000 << "] " << message << std::endl;
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

    std::random_device rd;  // Obtain a random number from hardware
    std::default_random_engine gen(rd());
    std::normal_distribution<>price_dist(args.commodity_price_mean,args.commodity_price_stand_dev);
    while(true)
    {
        //Generate a price for the commodity
        double price=price_dist(gen);

        log_time((std::string(args.commodity_name) + ": generating a new value: " + std::to_string(price)).c_str());

        // Wait for an empty slot
        semaphoreWait(empty);


         // Wait to lock the buffer (ensure mutual exclusion)
        log_time((std::string(args.commodity_name) + ": trying to get mutex on shared buffer").c_str());
        semaphoreWait(mutex);

        //place the price in the buffer
        snprintf(buffer->commodities[buffer->in],MAX_COMMODITY_NAME, "%s",args.commodity_name);
        buffer->prices[buffer->in]=price; // Store the price
        // Update 'in' index (circular buffer)
        buffer->in=((buffer->in)+1) % args.bounded_buffer_size;
        buffer->count++;


        // Unlock the buffer (release mutex)
        // semaphoreSignal();

        log_time((std::string(args.commodity_name) + ": placing price " + std::to_string(price) + " on shared buffer").c_str());

        // Signal that new data is available for consumers
        semaphoreSignal(mutex);
        semaphoreSignal(full);

        log_time((std::string(args.commodity_name) + ": sleeping for " + std::to_string(args.sleep_interval_ms) + " ms").c_str());
        //sleep_using_clock_gettime(args.sleep_interval_ms * 1000); // Sleep in milliseconds
        std::this_thread::sleep_for(std::chrono::milliseconds(args.sleep_interval_ms)); // Sleep

    }


}

int main(int arguments_count, char* arguments[])
{
    struct shared_buffer* buffer;
    // Setup shared memory
    int shmid=setupSharedMemory(MAX_BOUNDED_BUFFER_SIZE, &buffer);
    initializeSemaphore(MAX_BOUNDED_BUFFER_SIZE);

    // Parse command-line arguments
    Producer_ARGS args = parse_arguments(arguments_count, arguments);

    // Start producing
    producer(args, buffer);

    return 0;


}
