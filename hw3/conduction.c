#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"

#ifndef W
#define W 20                                    // Width
#endif
int main(int argc, char **argv) {
  
  MPI_Status status;
  MPI_Init(&argc, &argv);
  
  int L = atoi(argv[1]);                        // Length
  int iteration = atoi(argv[2]);                // Iteration
  srand(atoi(argv[3]));                         // Seed
  float d = (float) random() / RAND_MAX * 0.2;  // Diffusivity
  int *temp = malloc(L*W*sizeof(int));          // Current temperature
  int *next = malloc(L*W*sizeof(int));          // Next time step
  
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  for (int i = 0; i < L; i++) {
    for (int j = 0; j < W; j++) {
      temp[i*W+j] = random()>>3;
    }
  }
  //length size per thread
  int sub_length = L / size;
  //the leftmost 
  int start = sub_length * rank;
  //the rightmost
  int end  = start + sub_length;
  int all_balance = 0;
  int count = 0, balance = 0;
  int left[20];
  int right[20];
  memcpy(&left, (rank == 0)? temp : temp + (start-1)*W,  sizeof(int)*W);
  memcpy(&right, (rank == size-1)? temp+(L-1)*W : temp+(end*W),  sizeof(int)*W);

  while (iteration--) {     // Compute with up, left, right, down points
    balance = 1;
    count++;
    for (int i = start; i < end; i++){
      for (int j = 0; j < W; j++){
        
        float t = temp[i*W+j] / d;
        t += temp[i*W+j] * -4;
        if (i == start)
          t += left[j];
        else 
          t += temp[(i - 1) * W + j];

        if (i == end - 1)
          t += right[j];
        else 
          t += temp[(i + 1) * W + j];
        
        t += temp[i*W+(j - 1 <  0 ? 0 : j - 1)];
        t += temp[i*W+(j + 1 >= W ? j : j + 1)];
        t *= d;
        next[i*W+j] = t ;
        if (next[i*W+j] != temp[i*W+j]) {
          balance = 0;
        }
      }
    }
    MPI_Allreduce(&balance, &all_balance, 1, MPI_INT, MPI_BAND, MPI_COMM_WORLD);
    if (all_balance) 
      break;

    int *tmp = temp;
    temp = next;
    next = tmp;

    if (rank != 0){  
      MPI_Recv(&left, 20, MPI_INT, rank-1, 1, MPI_COMM_WORLD, &status);
      MPI_Send(temp+start*W, 20, MPI_INT, rank-1, 0, MPI_COMM_WORLD);
    }

    if (rank < size-1){   
      MPI_Send(temp+(end-1)*W, 20, MPI_INT, rank+1, 1, MPI_COMM_WORLD);
      MPI_Recv(&right, 20, MPI_INT, rank+1, 0, MPI_COMM_WORLD, &status);
        
    }
    if (rank == 0){
      memcpy(&left, temp, sizeof(int)*W);
    }
    
    if (rank == size-1){
      memcpy(&right, temp+(L-1)*W, sizeof(int)*W);
    }
  }

  int all_min ;
  int min = temp[start*W];
  for (int i = start; i < end; i++) {
    for (int j = 0; j < W; j++) {
      if (temp[i*W+j] < min) {
        min = temp[i*W+j];
      }
    }
  }
  //printf("min = %d\n", min);
  MPI_Allreduce(&min, &all_min, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
  printf("Size: %d*%d, Iteration: %d, Min Temp: %d\n", L, W, count, all_min);
  MPI_Finalize();
  return 0;
}