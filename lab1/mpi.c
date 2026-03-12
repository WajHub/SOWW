#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>
#include <math.h>

int isPrime(long N) {
  if(N==0 || N == 1) return 0;
    for (int i = 2; i <= sqrt(N); i++) {
        if (N % i == 0) {
            return 0;
        }
    }
    return 1;
}

int main(int argc,char **argv) {

  Args ins__args;
  parseArgs(&ins__args, &argc, argv);

  //program input argument
  long inputArgument = ins__args.arg; 

  struct timeval ins__tstart, ins__tstop;

  int myrank,nproc;
  
  MPI_Init(&argc,&argv);

  // obtain my rank
  MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
  // and the number of processes
  MPI_Comm_size(MPI_COMM_WORLD,&nproc);

  if(!myrank)
      gettimeofday(&ins__tstart, NULL);


  // run your computations here (including MPI communication)
  long result = 0;
  long resultFinal = 0;
  long sizeRange = inputArgument / nproc;

  long start = sizeRange * myrank;
  long end = sizeRange * (myrank  +1);

  for (long i = start; i <= end ; i++) {
    if(isPrime(i)){
      // printf("\nProcess %d Number: %ld", myrank,i);
      result++;
    }
  }

  MPI_Reduce (&result, &resultFinal, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);




  // synchronize/finalize your computations

  if (!myrank) {
    gettimeofday(&ins__tstop, NULL);
    printf("\nFinal Result %ld \n", resultFinal);
    ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);
  }
  
  MPI_Finalize();

}
