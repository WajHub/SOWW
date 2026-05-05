#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>
#include <sys/time.h>
#include "numgen.c"

__host__ void errorexit(const char *s)
{
  printf("\n%s", s);
  exit(EXIT_FAILURE);
}

__device__ int isPrime(long N)
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

__global__ void calculate(int *result, long upper_limit)
{
  int my_index = blockIdx.x * blockDim.x + threadIdx.x;
  if (my_index > 0 && my_index <= upper_limit - 2)
  {
    if (isPrime(my_index) && isPrime(my_index + 2))
    {
      result[my_index] = 1;
    }
    else
    {
      result[my_index] = 0;
    }
  }
}

int main(int argc, char **argv)
{

  Args ins__args;
  parseArgs(&ins__args, &argc, argv);

  int inputArgument = ins__args.arg;
  int threadsInBlock = 1024;
  int blocksInGrid = (inputArgument + threadsInBlock - 1) / threadsInBlock;

  struct timeval ins__tstart, ins__tstop;
  gettimeofday(&ins__tstart, NULL);

  // HOST - CPU
  int *hresults = (int *)malloc(inputArgument * sizeof(int));
  if (!hresults)
    errorexit("Error allocating memory on the host");

  // DEVICE - GPU
  int *dresults = NULL;
  // memory allocation on device (GPU)
  if (cudaSuccess != cudaMalloc((void **)&dresults, inputArgument * sizeof(int)))
    errorexit("Error allocating memory on the GPU");
  cudaMemset(dresults, 0, inputArgument * sizeof(int)); // CLEAR MEMORY

  // CALCULATE
  calculate<<<blocksInGrid, threadsInBlock>>>(dresults, inputArgument);
  if (cudaSuccess != cudaGetLastError())
    errorexit("Error during kernel launch");

  // COPY
  if (cudaSuccess != cudaMemcpy(hresults, dresults, inputArgument * sizeof(int), cudaMemcpyDeviceToHost))
    errorexit("Error copying results");

  // FINILIZE - SUM FLAGS
  int result = 0;
  for (int i = 0; i < inputArgument; i++)
  {
    result = result + hresults[i];
  }
  printf("\nThe final result is %d\n", result);

  gettimeofday(&ins__tstop, NULL);
  ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);

  // FREE RESOURCES
  free(hresults);
  if (cudaSuccess != cudaFree(dresults))
    errorexit("Error when deallocating space on the GPU");
}
