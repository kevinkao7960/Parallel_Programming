#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char* argv[]){
    clock_t start, end;
    start = clock();
    long long int number_of_tosses = atoi(argv[2]);

    int number_in_circle = 0;
    double x, y, distance_squared, pi_estimate;
    srand(time(NULL));
    for( long long int toss = 0; toss < number_of_tosses; toss++ ){
        x = (double)rand()/RAND_MAX*2.0 - 1.0;
        y = (double)rand()/RAND_MAX*2.0 - 1.0;
        distance_squared = x*x + y*y;
        if( distance_squared <= 1.0 ){
            number_in_circle++;
        }
    }
    
    pi_estimate = 4 * number_in_circle / ((double)number_of_tosses);
    printf("PI: %f\n", pi_estimate);
    end = clock();

    double cpu_time_used = ((double)(end-start)) / CLOCKS_PER_SEC;
    printf("CPU Time: %f\n", cpu_time_used);

    return 0;
}
