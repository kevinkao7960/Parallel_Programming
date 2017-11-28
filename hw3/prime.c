#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"

int isprime(long long int n) {
  long long int i,squareroot;
  if (n>10) {
    squareroot = (long long int) sqrt(n);
    for (i=3; i<=squareroot; i=i+2)
      if ((n%i)==0)
        return 0;
    return 1;
  }
  else
    return 0;
}

int main(int argc, char *argv[])
{
  long long int pc,       /* prime counter */
                foundone; /* most recent prime found */
  long long int n, limit;

  /* var for MPI */
  int my_rank, size, source, dest = 0, tag = 0;
  long long int local_pc = 0, start, end, interval, temp_found = 0;
  MPI_Status status;

  sscanf(argv[1],"%llu",&limit);
  printf("Starting. Numbers to be scanned= %lld\n",limit);

  //pc=4;     /* Assume (2,3,5,7) are counted here */
  MPI_Init(&argc, &argv); /* Let the system do what it needs to start up MPI */
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); /* Get my process rank */
  MPI_Comm_size(MPI_COMM_WORLD, &size); /* Find out how many processes are being used*/
  
  interval = (limit - 10) / size;
  start = interval * my_rank + 11;
  end = start + interval - 1;

  for (n = start; n <= end; n = n + 2) {
    if (isprime(n)) {
      local_pc++;
      foundone = n;
    }
  }

  /* Add up the results (local_pc) by each process*/
  if( my_rank == 0 ){
    pc = 4 + local_pc; /* Assume (2,3,5,7) are counted here*/
    for( source = 1; source < size; source++){
      MPI_Recv(&local_pc, 1, MPI_LONG_LONG_INT, source, tag, MPI_COMM_WORLD, &status);
      MPI_Recv(&temp_found, 1, MPI_LONG_LONG_INT, size - source, tag, MPI_COMM_WORLD, &status);
      if( temp_found > foundone ){
        foundone = temp_found;
      }
      pc = pc + local_pc;
    }
  }
  else{
    MPI_Send(&local_pc, 1, MPI_LONG_LONG_INT, dest, tag, MPI_COMM_WORLD);
    MPI_Send(&foundone, 1, MPI_LONG_LONG_INT, dest, tag, MPI_COMM_WORLD);
  }

  MPI_Finalize();
  printf("Done. Largest prime is %d Total primes %d\n",foundone,pc);

  return 0;
}
