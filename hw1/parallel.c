#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

long long int thread_count;
long long int number_in_circle = 0;
long long int number_of_tosses;
pthread_mutex_t mutex;

void* thread_toss(void* rank);
int main(int argc, char* argv[]){
    // calculate the program time
    clock_t start, end;
    start = clock();
    srand(time(NULL));

    // mutex initialize
    pthread_mutex_init(&mutex, NULL);

    // thread initialize
    long thread;
    pthread_t* thread_handles;
    thread_count = atoi(argv[1]);
    number_of_tosses = atoi(argv[2]);

    thread_handles = (pthread_t*)malloc(sizeof(pthread_t) * thread_count);

    for( thread = 0; thread < thread_count; thread++ ){
          pthread_create(&thread_handles[thread], NULL, thread_toss, NULL);
    }

    // wait for thread join to main thread
    for( thread = 0; thread < thread_count; thread++ ){
        pthread_join(thread_handles[thread], NULL);
    }

    // calculate pi
    long double pi_estimate;
    pi_estimate = 4 * number_in_circle / ((long double)number_of_tosses);

    end = clock();

    printf("PI: %Lf\n", pi_estimate);
    double cpu_time_used = ((double)(end-start)) / CLOCKS_PER_SEC;
    printf("CPU Time: %f\n", cpu_time_used);

    pthread_mutex_destroy(&mutex);
    free(thread_handles);

    return 0;
}

void* thread_toss(void* rank){
    long long int i;
    long double x, y, distance_squared;
    long long int my_n = number_of_tosses / thread_count;
    long long int tmp_sum = 0;

    for( i = 0; i < my_n; i++ ){
        x = (long double)rand()/RAND_MAX*2.0 - 1.0;
        y = (long double)rand()/RAND_MAX*2.0 - 1.0;
        distance_squared = x*x + y*y;
        if( distance_squared <= 1.0 ){
            tmp_sum++;
        }
    }

    // mutex for number_in_circle
    pthread_mutex_lock(&mutex);
    number_in_circle += tmp_sum;
    pthread_mutex_unlock(&mutex);
    // pthread_exit(NULL);

    return NULL;
}
