#include <stdio.h>
#include <math.h>
#include "mpi.h"

#define PI 3.1415926535

int main(int argc, char **argv)
{
  long long i, num_intervals;
  double rect_width, area, sum, x_middle;

  /* var for MPI */
  int my_rank, size, source, dest = 0, tag = 0;
  long long int start, end, interval_amount;
  double local_sum;
  MPI_Status status;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  sscanf(argv[1],"%llu",&num_intervals);

  rect_width = PI / num_intervals;
  interval_amount = num_intervals / size;
  local_sum = 0;
  start = 1 + my_rank * interval_amount;

  if( my_rank == (size-1) ){
    end = num_intervals + 1;
  }
  else{
    end = start + interval_amount;
  }

  for( i = start; i < end; i++ ){
    /* find the middle of the interval on the X-axis. */
    x_middle = (i - 0.5) * rect_width;
    area = sin(x_middle) * rect_width;
    local_sum = local_sum + area;
  }

  if( my_rank == 0 ){
    sum = local_sum;
    for( source = 1; source < size; source++){
      MPI_Recv(&local_sum, 1, MPI_DOUBLE, source, tag, MPI_COMM_WORLD, &status);
      sum = sum + local_sum;
    }
    printf("The total area is: %f\n", (float)sum);
  }
  else{
    MPI_Send(&local_sum, 1, MPI_DOUBLE, dest, tag, MPI_COMM_WORLD);
  }
  MPI_Finalize();

  return 0;
}
