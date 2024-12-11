#include <iostream>
#include <vector>
#include <string.h>
#include <thread>
#include "shared_memory.h"
#include <unistd.h> // For fork and usleep
#include <sys/wait.h> // For wait

#define MAX_BOUNDED_BUFFER_SIZE 40

// Commodities and store the last 5 prices of each
struct commodity
{
   std::string name;
   double price[5] = {0.00};
   double AvgPrice = 0.00; 
};

// Calculate average 
double average(double prices[5]){
    double avgPrice=0.00;
    for (int i = 0; i < 5; i++){
        avgPrice += prices[i];
    }
    return avgPrice/5;
}

// Modify the prices and the average
void modify(commodity &c ,double newPrice){
    for (int i = 4; i > 0; --i) {
        c.price[i] = c.price[i - 1];
    }
    c.price[0] = newPrice;
    c.AvgPrice = average(c.price);
}

// Display
void displayTable(std::vector<commodity>commodity){
    std::cout<<"\e[1;1H\e[2J";
    std::cout<<"+---------------------------------------------+"<<"\n";
    std::cout<<"|     Currency     |   Price   |   AvgPrice   |"<<"\n";
    std::cout<<"+---------------------------------------------+"<<"\n";
    for (auto & c : commodity){
        double lastPrice = c.price[0];
        double prevPrice = c.price[1];
        const char *arrow;
        const char* color;
        if (lastPrice > prevPrice){
            arrow = "↑";
            color = "\033[1;32m"; //green
        }
        else if (lastPrice < prevPrice){
            arrow = "↓";
            color = "\033[1;31m"; //red
        }
        else {
            arrow = "";
            color = "\033[1;34m"; //blue
        }
        printf("| %-16s | %s%7.2lf\033[0m   |  %s%7.2lf\033[0m%s     |\n" ,c.name.c_str() ,color ,lastPrice ,color ,c.AvgPrice ,arrow);
    }
    std::cout<<"+---------------------------------------------+"<<"\n";
}

void consumer(int bounded_buffer_size, struct shared_buffer* buffer){
    std::vector<commodity> commodities;
    std::string products[] = {"ALUMINIUM", "COPPER", "COTTON", "CRUDEOIL", "GOLD", "LEAD", "MENTHANOL", "NATURALGAS", "NICKEL", "SILVER", "ZINC"};

    // Add the product names to the vector
    for (int i = 0; i < 11; i++){
        commodity c;
        c.name = products[i];
        commodities.push_back(c);
    }

    while(true)
    {
        // Wait if full
        semaphoreWait(full);

        // Wait to lock the buffer (ensure mutual exclusion)
        semaphoreWait(mutex);

        // Read from buffer and update 'out' index (circular buffer)
        std::string name = buffer->commodities[buffer->out];
        double price = buffer->prices[buffer->out];

        buffer->out=((buffer->out)+1) % bounded_buffer_size;
        buffer->count--;
    
        // Signal that buffer is empty for producers
        semaphoreSignal(mutex);
        semaphoreSignal(empty);

        // Modify "commodity" with the new price
        for (auto & c : commodities){
            if(c.name == name){
                modify(c,price);
                break;
            }
        }
        // display the table
        displayTable(commodities); 
    }
}

int main(int argc, char** argv)
{
    if (argc != 2){
        std::cerr << "Usage: ./consumer <buffer_size>\n";
        exit(1);
    }

    int bounded_buffer_size = std::stoi(argv[1]);
    if (bounded_buffer_size > MAX_BOUNDED_BUFFER_SIZE) {
        std::cerr << "Error: Buffer size exceeds maximum allowed (" << MAX_BOUNDED_BUFFER_SIZE << ").\n";
        exit(1);
    }

    // Setup shared memory
    struct shared_buffer* buffer;
    // setupSharedMemory(MAX_BOUNDED_BUFFER_SIZE, &buffer);
    int shmid = setupSharedMemory(bounded_buffer_size, &buffer);
    initializeSemaphore(0);

    // consumer(bounded_buffer_size, buffer);
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        cleanupSharedMemory(shmid, buffer);
        cleanupSemaphores();
        exit(1);
    } else if (pid == 0) {
        // Child process: Consumer
        consumer(bounded_buffer_size, buffer);
        exit(0);
    } else {
        // Parent process: Wait for the consumer process to finish
        wait(NULL);

        // Cleanup resources
        cleanupSharedMemory(shmid, buffer);
        cleanupSemaphores();
    }

return 0;
}