#include <stdio.h>
#include <mpi.h>
#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#define PRECISION 0.000001
#define RANGESIZE 10
#define DATA 0
#define RESULT 1
#define FINISH 2

typedef struct
{
  double range[2];
  double result;
  MPI_Request send_req;
  MPI_Request recv_req;
} SlaveHandler;

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
    if(next_number % 2 == 0) {
      next_number++;
    }
    if (next_number <= upper_limit && isPrime(next_number))
    {
      count++;
    }
  }
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
  int i;
  MPI_Status status;

  // Initialize MPI, find out my rank, find out the number of processes in MPI_COMM_WORLD
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  MPI_Comm_size(MPI_COMM_WORLD, &proccount);
  if (proccount < 2){
    printf("Run with at least 2 processes");
    MPI_Finalize();
    return -1;
  }

  if (((b - a) / RANGESIZE) < 2 * (proccount - 1)){
    printf("More subranges needed");
    MPI_Finalize();
    return -1;
  }

  SlaveHandler *handlers = malloc(sizeof(SlaveHandler) * (proccount - 1));
  MPI_Request *requests = malloc(sizeof(MPI_Request) * (proccount - 1));
  if (handlers == NULL || requests == NULL){
    perror("malloc");
    return -1;
  }

  // master will distribute the data and slave processes will perform computations
  if (myrank == 0){
    gettimeofday(&ins__tstart, NULL);
    range[0] = a;

    // INITAL SENDING
    for (i = 1; i < proccount; i++){
      int handler_index = i - 1;
      range[1] = range[0] + RANGESIZE - 1;
      MPI_Send(range, 2, MPI_DOUBLE, i, DATA, MPI_COMM_WORLD); // send it to process i

      MPI_Irecv(&handlers[handler_index].result, 1, MPI_DOUBLE, i, RESULT, MPI_COMM_WORLD, &handlers[handler_index].recv_req);
      range[0] = range[1] + 1;
    }

    // SENDING next Non-blocking messages
    for (i = 1; i < proccount; i++){
      int handler_index = i - 1;
      if (range[0] <= b){
        range[1] = range[0] + RANGESIZE - 1;
        if (range[1] > b){
          range[1] = b;
        }

        handlers[handler_index].range[0] = range[0];
        handlers[handler_index].range[1] = range[1];

        MPI_Isend(handlers[handler_index].range, 2, MPI_DOUBLE, i, DATA, MPI_COMM_WORLD, &handlers[handler_index].send_req);
        range[0] = range[1] + 1;
      }
      else{
        handlers[handler_index].send_req = MPI_REQUEST_NULL;
      }
    }

    // MASTER MAINLOOP
    while (range[0] <= b){
      int completed_index;

      for (int j = 0; j < proccount - 1; j++){
        requests[j] = handlers[j].recv_req;
      }

      // Wait for result
      MPI_Waitany(proccount - 1, requests, &completed_index, &status);

      int target_slave = completed_index + 1;
      result += handlers[completed_index].result; // Add part-resutl to Final Result

      // Ensure - previoues messagesd was finished
      MPI_Wait(&handlers[completed_index].send_req, MPI_STATUS_IGNORE);

      range[1] = range[0] + RANGESIZE - 1;
      if (range[1] > b){
        range[1] = b;
      }
        
      handlers[completed_index].range[0] = range[0];
      handlers[completed_index].range[1] = range[1];

      MPI_Isend(handlers[completed_index].range, 2, MPI_DOUBLE, target_slave, DATA, MPI_COMM_WORLD, &handlers[completed_index].send_req);

      // Refresh Irecv to read new data
      MPI_Irecv(&handlers[completed_index].result, 1, MPI_DOUBLE, target_slave, RESULT, MPI_COMM_WORLD, &handlers[completed_index].recv_req);

      range[0] = range[1] + 1;
    }

    for (i = 0; i < proccount - 1; i++){
      MPI_Wait(&handlers[i].recv_req, MPI_STATUS_IGNORE);
      result += handlers[i].result;
    }

    for (i = 1; i < proccount; i++){
      MPI_Recv(&resulttemp, 1, MPI_DOUBLE, i, RESULT, MPI_COMM_WORLD, &status);
      result += resulttemp;
    }

    // Finish (after receiving all messages)
    for (i = 1; i < proccount; i++){
      MPI_Send(NULL, 0, MPI_DOUBLE, i, FINISH, MPI_COMM_WORLD);
    }
    gettimeofday(&ins__tstop, NULL);
    ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);
  }

  else{
    double next_range[2];
    double results[2];
    int res_idx = 0;
    MPI_Request recv_req = MPI_REQUEST_NULL;
    MPI_Request send_req = MPI_REQUEST_NULL;

    MPI_Recv(range, 2, MPI_DOUBLE, 0, DATA, MPI_COMM_WORLD, &status);

    while (status.MPI_TAG != FINISH){
      MPI_Irecv(next_range, 2, MPI_DOUBLE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &recv_req);

      results[res_idx] = countTwinPrimes(range[0], range[1], b);

      if (send_req != MPI_REQUEST_NULL){
        MPI_Wait(&send_req, MPI_STATUS_IGNORE);
      }

      MPI_Isend(&results[res_idx], 1, MPI_DOUBLE, 0, RESULT, MPI_COMM_WORLD, &send_req);

      if(res_idx == 0 ){
        res_idx = 1;
      }
      else {
        res_idx = 0;
      }

      MPI_Wait(&recv_req, &status);
      range[0] = next_range[0];
      range[1] = next_range[1];
    }

    if (send_req != MPI_REQUEST_NULL){
      MPI_Wait(&send_req, MPI_STATUS_IGNORE);
    }
      
  }

  if (!myrank)
  {
    printf("\nFinal Result %f \n", result);
  }

  free(handlers);
  free(requests);

  MPI_Finalize();
  return 0;
}



// Master wysyła pierwsze porcje zadań, a zaraz po tym wysyła nieblokująco "zapasowe" porcje