#include <iostream>
#include <vector>
#include <string.h>
#include <unistd.h> // For fork and usleep
#include <fcntl.h>  // for O_CREAT
#include <sys/stat.h> // for mode constants
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>

using namespace std;

#define MAX_BOUNDED_BUFFER_SIZE 40
#define MAX_COMMODITY_NAME 11

#define SEM_KEY1 0x54321
#define SEM_KEY2 0x54322
#define SEM_KEY3 0x54323

struct shared_buffer {

    char commodities[MAX_BOUNDED_BUFFER_SIZE][MAX_COMMODITY_NAME];
    double prices[MAX_BOUNDED_BUFFER_SIZE]; // prices associated with the commodities
    int in; // points to the next position where the producer will insert a new commodity
    int out; // points to the next position where the consumer will remove a commodity
    int count; // keeps track of how many items are currently in the buffer
};

// Commodities and store the last 5 prices of each
struct commodity
{
   string name;
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
void displayTable(vector<commodity>commodity){
    cout<<"\e[1;1H\e[2J";
    cout<<"+-------------------------------------------------+"<<"\n";
    cout<<"|     Currency     |    Price    |    AvgPrice    |"<<"\n";
    cout<<"+-------------------------------------------------+"<<"\n";
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
        printf("| %-16s |  %s%7.2lf\033[0m    |   %s%7.2lf\033[0m%-1s     |\n" ,c.name.c_str() ,color ,lastPrice ,color ,c.AvgPrice ,arrow);
    }
    cout<<"+-------------------------------------------------+"<<"\n";
}


void consumer(int bounded_buffer_size, struct shared_buffer* buffer) {
    vector<commodity> commodities;
    string products[] = {"ALUMINIUM", "COPPER", "COTTON", "CRUDEOIL", "GOLD", "LEAD", "MENTHANOL", "NATURALGAS", "NICKEL", "SILVER", "ZINC"};

    // Add the product names to the vector
    for (int i = 0; i < 11; i++) {
        commodity c;
        c.name = products[i];
        commodities.push_back(c);
    }

    struct sembuf sem_buf;

    // Semaphore for mutual exclusion
    int emptyId = semget(SEM_KEY1, 1, 0666);
    int fullId = semget(SEM_KEY2, 1, 0666);
    int mutexId = semget(SEM_KEY3, 1, 0666);

    if (emptyId == -1 || fullId == -1 || mutexId == -1) {
        perror("semget failed");
        exit(1);
    }

    // Initialize semaphores
    semctl(emptyId, 0, SETVAL, bounded_buffer_size);
    semctl(fullId, 0, SETVAL, 0);
    semctl(mutexId, 0, SETVAL, 1);

    while (true) {
        // Wait if full (if buffer is empty, consumer will wait)
        sem_buf = {0, -1, 0};
        semop(fullId, &sem_buf, 1);

        // Wait to lock the buffer (ensure mutual exclusion)
        semop(mutexId, &sem_buf, 1);

        // Check if the buffer has any items to consume
        if (buffer->count > 0) {
            // Read from buffer and update 'out' index (circular buffer)
            std::string name = buffer->commodities[buffer->out];
            double price = buffer->prices[buffer->out];

            buffer->out = (buffer->out + 1) % bounded_buffer_size;
            buffer->count--;

            // Unlock the buffer (release mutex)
            sem_buf = {0, 1, 0};
            semop(mutexId, &sem_buf, 1);

            // Signal that buffer is empty for producers
            semop(emptyId, &sem_buf, 1);

            // Modify commodity with the new price
            for (auto &c : commodities) {
                if (c.name == name) {
                    modify(c, price);
                    break;
                }
            }

            // Display the table
            displayTable(commodities);
        } else {
            // If no items in buffer, print debug message
            cout << "Buffer is empty, waiting for producer..." << endl;
        }

        usleep(100000); // Sleep for 0.1 seconds
    }
}

int main(int argc, char** argv)
{
    if (argc != 2){
        cerr << "Usage: ./consumer <buffer_size>\n";
        exit(1);
    }

    int bounded_buffer_size = std::stoi(argv[1]);
    if (bounded_buffer_size > MAX_BOUNDED_BUFFER_SIZE) {
        cerr << "Error: Buffer size exceeds maximum allowed (" << MAX_BOUNDED_BUFFER_SIZE << ").\n";
        exit(1);
    }

    // Setup shared memory
    key_t key = ftok("file", 65);
    if (key == -1) {
        perror("ftok failed");
        exit(1);
    }

    // Setup shared memory
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

    // Start consuming
    consumer(bounded_buffer_size, buffer);

    // Detach from shared memory
    shmdt(buffer);

    // Destroy the shared memory
    shmctl(shmID, IPC_RMID, NULL);

return 0;
}