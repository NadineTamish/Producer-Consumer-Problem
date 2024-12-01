#include <iostream>
#include <random>
#include <cstring>
#include <ctime>
#include <unistd.h> //for sleep fn
#include <semaphore.h>
#include <fcntl.h>  // for O_CREAT
#include <sys/stat.h> // for mode constants
#include "shared_memory.h"  // Assuming shared memory and buffer are defined her

#define MAX_COMMODITY_NAME 10
#define MAX_BOUNDED_BUFFER_SIZE 40


struct Producer_ARGS{

    char commodity_name[MAX_COMMODITY_NAME];
    double commodity_price_mean;
    double commodity_price_stand_dev;
    int sleep_interval_ms;
    int bounded_buffer_size;
};


Producer_ARGS parse_arguments(int arguments,char *args[])
{

    if(arguments!=6)
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
    std::strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", std::localtime(&ts.tv_sec));
    std::cerr << "[" << time_buffer << "." << ts.tv_nsec / 1000000 << "] " << message << std::endl;
}

