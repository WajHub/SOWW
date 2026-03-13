#include <stdio.h>
#include <mpi.h>
#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#define PRECISION 0.000001
#define RANGESIZE 100
#define DATA 0
#define RESULT 1
#define FINISH 2

int isPrime(long N) {
    if (N < 2) return 0;
    if (N == 2) return 1;
    if (N % 2 == 0) return 0;
    for (long i = 3; i * i <= N; i += 2) {
        if (N % i == 0) return 0;
    }
    return 1;
}

double countTwinPrimes(long a, long b, long upper_limit) {
    double count = 0;
    int previous_is_prime = 0;
    // printf("Ranges: %ld %ld \n", a, b);

    if(a%2== 0) a++;
    for (long i = a; i <= b; i+=2) {
        if (isPrime(i)) {
          if(previous_is_prime == 1) {
            count++;
            // printf(" ---- Twin Primes: %ld %ld \n", i, i-2);
          }
          else {
            previous_is_prime = 1;
          }
          
        }
        else{
          previous_is_prime = 0;
        }
    }

    if(previous_is_prime==1) {
      long next_number = b + 1;
      if(upper_limit % 2 == 0) {
        next_number++;
      }
      if(next_number<= upper_limit && isPrime(next_number)){
        count++;
      }
    }
    return count;

    // if(a%2== 0) a++;
    // for (long i = a; i <= b; i+=2) {
    //     if (isPrime(i) && isPrime(i + 2)) {
    //       printf(" ---- Twin Primes: %ld %ld \n", i, i+2);
    //       count++;
    //     }
    // }
    return count;
}

int main(int argc, char **argv)
{
  Args ins__args;
  parseArgs(&ins__args, &argc, argv);
  struct timeval ins__tstart, ins__tstop;
  int myrank, proccount;
  double a = 0;
  double b = ins__args.arg;
  double range[2];
  double result = 0, resulttemp;
  int sentcount = 0;
  int i;
  MPI_Status status;

  // Initialize MPI
  MPI_Init(&argc, &argv);

  // find out my rank
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

  // find out the number of processes in MPI_COMM_WORLD
  MPI_Comm_size(MPI_COMM_WORLD, &proccount);

  if (proccount < 2)
  {
    printf("Run with at least 2 processes");
    MPI_Finalize();
    return -1;
  }

  if (((b - a) / RANGESIZE) < 2 * (proccount - 1))
  {
    printf("More subranges needed");
    MPI_Finalize();
    return -1;
  }

  // now the master will distribute the data and slave processes will perform computations
  if (myrank == 0)
  {
    gettimeofday(&ins__tstart, NULL);
    range[0] = a;

    // first distribute some ranges to all slaves
    for (i = 1; i < proccount; i++)
    {
      range[1] = range[0] + RANGESIZE - 1;
      MPI_Send(range, 2, MPI_DOUBLE, i, DATA, MPI_COMM_WORLD); // send it to process i
      sentcount++;
      range[0] = range[1] + 1;
    }
    do
    {
      // distribute remaining subranges to the processes which have completed their parts
      MPI_Recv(&resulttemp, 1, MPI_DOUBLE, MPI_ANY_SOURCE, RESULT, MPI_COMM_WORLD, &status);
      result += resulttemp;
      // check the sender and send some more data
      range[1] = range[0] + RANGESIZE - 1;
      if (range[1] > b)
        range[1] = b;
      MPI_Send(range, 2, MPI_DOUBLE, status.MPI_SOURCE, DATA,
               MPI_COMM_WORLD);
      range[0] = range[1] + 1;
    } while (range[1] < b);
    // now receive results from the processes
    for (i = 0; i < (proccount - 1); i++)
    {
      MPI_Recv(&resulttemp, 1, MPI_DOUBLE, MPI_ANY_SOURCE, RESULT, MPI_COMM_WORLD, &status);
      result += resulttemp;
    }
    // shut down the slaves
    for (i = 1; i < proccount; i++)
    {
      MPI_Send(NULL, 0, MPI_DOUBLE, i, FINISH, MPI_COMM_WORLD);
    }
    gettimeofday(&ins__tstop, NULL);
    ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);

  }
  else
  {
    // SLAVE
    do
    {
      MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

      if (status.MPI_TAG == DATA)
      {
        MPI_Recv(range, 2, MPI_DOUBLE, 0, DATA, MPI_COMM_WORLD, &status);
        // compute my part
        resulttemp = countTwinPrimes(range[0], range[1], b);
        // send the result back
        MPI_Send(&resulttemp, 1, MPI_DOUBLE, 0, RESULT, MPI_COMM_WORLD);
      }
    } while (status.MPI_TAG != FINISH);
  }
  if(!myrank){
        printf("\nFinal Result %f \n", result);
  }

  MPI_Finalize();
  return 0;
}









// Liczba prcoesów to 1 master i (n-1) slave'ow!!!!
// Nie sprawdzamy dwa razy tych samych liczb, iterujemy tylko po nieparzystych liczb.
// Kolejny zakres ma range[0] = range[1] + 1 (czyli kolejna liczba od poprzedenigo zakresu)