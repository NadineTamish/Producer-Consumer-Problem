#define _POSIX_C_SOURCE 200112L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define GRID_SIZE 20
#define NUM_THREADS 4
#define GENERATIONS 32

int grid[GRID_SIZE][GRID_SIZE];
int next_gen_grid[GRID_SIZE][GRID_SIZE];
pthread_barrier_t barrier;

typedef struct
{
    int start, end;
} state;

void print_grid()
{
    system("clear");
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            if (grid[i][j] == 1)
            {
                printf("# ");
            }
            else
            {
                printf("  ");
            }
        }
        printf("\n");
    }
    usleep(500000); // sleep(500000)
}

// int count_live_neigbours(start,end,i,j){
//     int live_count=0;

//     for(int i=start; i<=end; i++){
//         for(int j=0; j<GRID_SIZE; j++){
//             for(int hor=-1; hor<=1; hor++){
//                 for(int ver=-1; ver<=1; ver++){
//                     if(hor==0 && ver==0)
//                         continue;

//                     if(i+hor>=0 || i+hor<GRID_SIZE
//                     || j+ver>=0 || j+ver<GRID_SIZE){
//                         if(next_gen_grid[i+hor][j+ver]==1){
//                             live_count++;
//                         }
//                     }
//                 }

//             }
//         }
//     }
//     return live_count;
// }

// Function to compute next generation of Game of Life
void *compute_next_gen(void *arg)
{
    int live_count=0;
    state *args = (state *)arg;
    int start = args->start;
    int end = args->end;

    for (int i = start; i <= end; i++){
        for (int j = 0; j < GRID_SIZE; j++){
            live_count=0;
            for (int hor = -1; hor <= 1; hor++){
                for (int ver = -1; ver <= 1; ver++){
                    if (hor == 0 && ver == 0)
                        continue;

                    if ((i+hor) >= 0 && (i+hor) < GRID_SIZE && (j+ver) >= 0 && (j+ver) < GRID_SIZE)
                    {
                        if (next_gen_grid[(i+hor)][(j+ver)] == 1)
                            live_count++;
                    }
                }
            }
            // birth rule
            if (next_gen_grid[i][j] == 0){
                if (live_count == 3){
                    next_gen_grid[i][j] = 1;
                }
            }

            if (next_gen_grid[i][j] == 1){
                // Survival Rule
                if (live_count == 2 || live_count == 3)
                {
                    next_gen_grid[i][j] = 1;
                }
                // Death Rule
                if (live_count < 2 || live_count > 3)
                {
                    next_gen_grid[i][j] = 0;
                }
            }
        }
    }

    int b = pthread_barrier_wait(&barrier); // wait for all threads to arrive at barrier
    //debugging
    // int errno = b;
    // perror("pthread_barrier_wait");
    // exit(1);

    // update next gen
    if(b == PTHREAD_BARRIER_SERIAL_THREAD){
        for (int i = 0; i < GRID_SIZE; i++){
            for (int j = 0; j < GRID_SIZE; j++){
                grid[i][j] = next_gen_grid[i][j];
            }
        }
        print_grid();
    }
    
    free(arg);

}

void initialize_grid(int grid[GRID_SIZE][GRID_SIZE])
{
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            grid[i][j] = 0; // Set every cell to 0 (dead)
        }
    }
}
void initialize_patterns(int grid[GRID_SIZE][GRID_SIZE])
{

    initialize_grid(grid);

    // Initialize Still Life (Square) at top-left
    grid[1][1] = 1;
    grid[1][2] = 1;
    grid[2][1] = 1;
    grid[2][2] = 1;

    // Initialize Oscillator (Blinker) in the middle
    grid[5][6] = 1;
    grid[6][6] = 1;
    grid[7][6] = 1;

    // Initialize Spaceship (Glider) in the bottom-right
    grid[10][10] = 1;
    grid[11][11] = 1;
    grid[12][9] = 1;
    grid[12][10] = 1;
    grid[12][11] = 1;
    // grid[15][15] = 1;
    // grid[16][16] = 1;
    // grid[17][14] = 1;
    // grid[17][15] = 1;
    // grid[17][16] = 1;
}

int main()
{
    initialize_patterns(grid);
    initialize_patterns(next_gen_grid);
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);
    pthread_t threads[NUM_THREADS];
    
    int passed_generations = 0;
    while (passed_generations <= GENERATIONS)
    {
        int rows = 0;
        for (int i = 0; i < NUM_THREADS; i++){
            state *args = (state *)malloc(sizeof(state));
            args->start = rows;
            rows += 5;
                // printf("%d\n",(end-NUM_THREADS)/(NUM_THREADS));
            if(i==NUM_THREADS-1){
                args->end = GRID_SIZE-1;
            }
            else{
                args->end = rows-1;
            }
            pthread_create(&threads[i], NULL, compute_next_gen, args); // check indexing of thread
        }

        for (int i = 0; i < NUM_THREADS; i++){
                pthread_join(threads[i], NULL);
        }
        
        printf("%d",passed_generations++);
    
    }

    pthread_barrier_destroy(&barrier);
    
    return 0;
}
