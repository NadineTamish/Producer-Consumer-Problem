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

typedef struct{
    int start,end;
}state;

void print_grid() {
    system("clear"); 
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j] == 1) {
                printf("# ");
            } else {
                printf("  ");
            }
        }
        printf("\n");
    }
    usleep(500000); //sleep(500000) 
}

int count_live_neigbours(start,end,i,j){
    int live_count=0;
   
    //don't forget handling out of bounds grid
    for(int i=start; i<=end; i++){
        for(int j=0; j<GRID_SIZE; j++){
            for(int hor=-1; hor<=1; hor++){
                for(int ver=-1; ver<=1; ver++){
                    if(i+hor>=0 || i+hor<GRID_SIZE
                    || j+ver>=0 || j+ver<GRID_SIZE){
                        if(next_gen_grid[i+hor][j+ver]==1){
                            live_count++;
                        }
                    }
                }

            }
        }
    }
    return live_count;
}

// Function to compute next generation of Game of Life
void* compute_next_gen(void* arg) {
    
    //add pthread_barrier_wait(&barrier) after thread computation done 
    state *args=(state*) arg;
    int start = args->start;
    int end = args->end;

    for(int i=start; i<=end; i++){
        for(int j=0; j<GRID_SIZE; j++){
            //birth rule
            if(next_gen_grid[i][j]==0){
                if(count_live_neigbours(start,end,i,j)==3){
                    next_gen_grid[i][j]=1;
                }
                else{
                    next_gen_grid[i][j]=0;
                }     
            }

            if(next_gen_grid[i][j]==1){
                int live_count=count_live_neigbours(start,end,i,j);
                //Survival Rule
                if(live_count==2|| live_count==3){
                    next_gen_grid[i][j]=1;
                }
                //Death Rule
                if(live_count<2 || live_count>3){
                    next_gen_grid[i][j]=0;
                }
            }
        }
    }
    
    
}


void initialize_grid(int grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = 0;  // Set every cell to 0 (dead)
        }
    }
    }
void initialize_patterns(int grid[GRID_SIZE][GRID_SIZE]) {
    
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
}

int main() {
    initialize_patterns(grid) ;
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);
    pthread_t threads[NUM_THREADS];

    state *args =(state*)malloc(sizeof(args)); //

    int rows =0, start, end;

    for(int i=0; i<NUM_THREADS; i++){

        if(i==(NUM_THREADS)+rows){
            args->start=rows;
            args->end=(NUM_THREADS)+rows;
            rows+=5;
            pthread_create(threads[(end-NUM_THREADS)/(NUM_THREADS)],NULL,compute_next_gen,args); //check indexing of thread
        }

    }

    pthread_barrier_destroy(&barrier);
    return 0;
}
