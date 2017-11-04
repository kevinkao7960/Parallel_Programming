#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>

long long int thread_count;
long long int number_in_circle = 0;
long long int thread_toss_num;
long long int* partial_sum;
pthread_mutex_t mutex;

void* thread_toss(void* rank);
int main(int argc, char* argv[]){
    if( argc != 3 ){
        fprintf(stderr, "you should put another two arguments!\n");
        exit(1);
    }


    // calculate the program time
    struct timeval t1, t2;

    srand(time(NULL));

    // mutex initialize
    pthread_mutex_init(&mutex, NULL);

    // thread initialize
    long thread;
    void* status;
    pthread_t* thread_handles;
    thread_count = atoi(argv[1]);
    partial_sum = (long long int*)malloc(sizeof(long long int) * thread_count);
    
    long long int number_of_tosses = atoi(argv[2]);
    thread_toss_num = number_of_tosses / thread_count;

    thread_handles = (pthread_t*)malloc(sizeof(pthread_t) * thread_count);
    gettimeofday(&t1, NULL);
    for( thread = 0; thread < thread_count; thread++ ){
        partial_sum[thread] = 0;
        pthread_create(&thread_handles[thread], NULL, thread_toss, (void*)thread);
    }

    // wait for thread join to main thread
    for( thread = 0; thread < thread_count; thread++ ){
        pthread_join(thread_handles[thread], NULL);
        number_in_circle += partial_sum[thread];
    }

    gettimeofday(&t2, NULL);

    pthread_mutex_destroy(&mutex);
    free(thread_handles);
    free(status);
    // calculate pi
    long double pi_estimate;
    pi_estimate = 4 * number_in_circle / ((long double)number_of_tosses);



    printf("PI: %Lf\n", pi_estimate);
    double elapsed_time = (t2.tv_sec - t1.tv_sec) * 1000;
    elapsed_time += (t2.tv_usec - t1.tv_usec) / 1000;

    double cpu_time_used = elapsed_time / 1000;
    printf("CPU Time: %lf\n", cpu_time_used);


    return 0;
}

void* thread_toss(void* rank){
    long long int i;
    long my_rank = (long)rank;
    double x, y;
    long long int tmp_sum = 0;

    for( i = 0; i < thread_toss_num; i++ ){
        x = (double)rand()/RAND_MAX*2.0 - 1.0;
        y = (double)rand()/RAND_MAX*2.0 - 1.0;
        if(  ((x*x) + (y*y))<= 1.0 ){
            tmp_sum++;
        }
    }

    partial_sum[my_rank] = tmp_sum;
    return NULL;
}
