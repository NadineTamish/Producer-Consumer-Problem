#include <iostream>
#include <vector>
#include <string.h>
#include "shared_memory.h"

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
        std::string arrow;
        if (lastPrice > prevPrice){
            arrow = "↑";
        }
        else if (lastPrice < prevPrice){
            arrow = "↓";
        }
        else {
            arrow = "";
        }
        std::cout << "|     %s     |   %7.2lf   |   %7.2lf %s   |\n" << c.name << lastPrice << c.AvgPrice << arrow;
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
    setupSharedMemory(MAX_BOUNDED_BUFFER_SIZE, &buffer);
    initializeSemaphore(0);

    consumer(bounded_buffer_size, buffer);

return 0;
}