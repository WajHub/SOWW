#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <omp.h>
#include "numgen.c"

int isPrime(long N)
{
  if (N < 2)
    return 0;
  if (N == 2)
    return 1;
  if (N % 2 == 0)
    return 0;
  for (long i = 3; i * i <= N; i += 2)
  {
    if (N % i == 0)
      return 0;
  }
  return 1;
}


int main(int argc,char **argv) {


  Args ins__args;
  parseArgs(&ins__args, &argc, argv);

  //set number of threads
  omp_set_num_threads(ins__args.n_thr);
  
  //program input argument
  long inputArgument = ins__args.arg; 
  // unsigned long int *numbers = (unsigned long int*)malloc(inputArgument * sizeof(unsigned long int));
  // numgen(inputArgument, numbers);

  struct timeval ins__tstart, ins__tstop;
  gettimeofday(&ins__tstart, NULL);
  
  // run your computations here (including OpenMP stuff)

  long result = 0;

  printf("Value at the begining is %ld \n",result);
  printf("Arguments %ld \n",inputArgument);

  //parallel for loop - all threads execute in parallel different parts of iterations
  #pragma omp parallel for reduction(+:result) // reduction allow to execute operation on chosen variable in all threads
  for(int i=3;i<inputArgument-1;i++) {
    if (isPrime(i) && isPrime(i+2)) {
        result++;
        // printf(" ---- Twin Primes: %d %d \n", i, (i+2));
    }
    
  }

  // synchronize/finalize your computations
  gettimeofday(&ins__tstop, NULL);
  ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);


  printf("Result %ld \n",result);

}
