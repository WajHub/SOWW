#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>
#include <omp.h>
#include "numgen.c"

#define RESULT 1

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

double countTwinPrimes(long a, long b, long upper_limit)
{
  double count = 0;
  int previous_is_prime = 0;
  // printf("Ranges: %ld %ld \n", a, b);

  if (a % 2 == 0)
    a++;
  for (long i = a; i <= b; i += 2)
  {
    if (isPrime(i))
    {
      if (previous_is_prime == 1)
      {
        count++;
        printf(" ---- Twin Primes: %ld %ld \n", i, i - 2);
      }
      else
      {
        previous_is_prime = 1;
      }
    }
    else
    {
      previous_is_prime = 0;
    }
  }

  if (previous_is_prime == 1)
  {
    long next_number = b + 1;
    if (next_number % 2 == 0)
    {
      next_number++;
    }
    if (next_number <= upper_limit && isPrime(next_number))
    {
      count++;
    }
  }
  return count;
}

void work(long start, long step, long upper_limit)
{
  double local_result = 0;

#pragma omp parallel for reduction(+ : local_result) // reduction allow to execute operation on chosen variable in all threads
  for (int i = start; i < upper_limit - 1; i = i + step)
  {
    if (isPrime(i) && isPrime(i + 2))
    {
      // if (i > 5)
      // {
      //   i = i + 2;
      // }
      local_result++;
      // printf(" ---- Twin Primes: %d %d \n", i, (i + 2));
    }
  }
  MPI_Send(&local_result, 1, MPI_DOUBLE, 0, RESULT, MPI_COMM_WORLD);
}


int main(int argc, char **argv)
{

  Args ins__args;
  parseArgs(&ins__args, &argc, argv);

  // set number of threads
  omp_set_num_threads(ins__args.n_thr);

  MPI_Status status;

  // program input argument
  long inputArgument = ins__args.arg;

  struct timeval ins__tstart, ins__tstop;

  int threadsupport;
  int myrank, nproc;
  unsigned long int *numbers;
  // Initialize MPI with desired support for multithreading -- state your desired support level

  MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &threadsupport);

  if (threadsupport < MPI_THREAD_FUNNELED)
  {
    printf("\nThe implementation does not support MPI_THREAD_FUNNELED, it supports level %d\n", threadsupport);
    MPI_Finalize();
    return -1;
  }

  // obtain my rank
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  // and the number of processes
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  if (!myrank)
  {
    gettimeofday(&ins__tstart, NULL);
    numbers = (unsigned long int *)malloc(inputArgument * sizeof(unsigned long int));
    numgen(inputArgument, numbers);
  }
  // run your computations here (including MPI communication and OpenMP stuff)
  // int local_range = inputArgument / nproc;
  // int local_start = local_range * myrank;
  // int local_end = local_start + local_range;
  long start = myrank * 2 + 1;
  long step = nproc * 2;
  long upper_limit = inputArgument;
  double total_result = 0;

  work(start, step, upper_limit);

  // synchronize/finalize your computations
  if (!myrank)
  {
    double resulttemp;
    for (int i = 0; i < nproc; i++)
    {
      MPI_Recv(&resulttemp, 1, MPI_DOUBLE, i, RESULT, MPI_COMM_WORLD, &status);
      printf("\nReceived result %f for process %d\n", resulttemp, i);
      // fflush (stdout);
    }
  }

  if (!myrank)
  {
    gettimeofday(&ins__tstop, NULL);
    ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);
  }

  MPI_Finalize();
}
