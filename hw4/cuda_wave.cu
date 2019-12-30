/**********************************************************************
 * DESCRIPTION:
 *   Serial Concurrent Wave Equation - C Version
 *   This program implements the concurrent wave equation
 *********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAXPOINTS 1000000
#define MAXSTEPS 1000000
#define MINPOINTS 20
#define PI 3.14159265

void check_param(void);
//void init_line(void);

void printfinal (void);

int nsteps,                 	/* number of time steps */
    tpoints, 	     		/* total points along string */
    rcode;                  	/* generic return code */
float  values[MAXPOINTS+2], 	/* values at time t */
       oldval[MAXPOINTS+2], 	/* values at time (t-dt) */
       newval[MAXPOINTS+2]; 	/* values at time (t+dt) */


/**********************************************************************
 *	Checks input values from parameters
 *********************************************************************/
void check_param(void)
{
   char tchar[20];

   /* check number of points, number of iterations */
   while ((tpoints < MINPOINTS) || (tpoints > MAXPOINTS)) {
      printf("Enter number of points along vibrating string [%d-%d]: "
           ,MINPOINTS, MAXPOINTS);
      scanf("%s", tchar);
      tpoints = atoi(tchar);
      if ((tpoints < MINPOINTS) || (tpoints > MAXPOINTS))
         printf("Invalid. Please enter value between %d and %d\n", 
                 MINPOINTS, MAXPOINTS);
   }
   while ((nsteps < 1) || (nsteps > MAXSTEPS)) {
      printf("Enter number of time steps [1-%d]: ", MAXSTEPS);
      scanf("%s", tchar);
      nsteps = atoi(tchar);
      if ((nsteps < 1) || (nsteps > MAXSTEPS))
         printf("Invalid. Please enter value between 1 and %d\n", MAXSTEPS);
   }

   printf("Using points = %d, steps = %d\n", tpoints, nsteps);

}

/**********************************************************************
 *     Initialize points on line
 *********************************************************************/
// void init_line(void)
// {
//    int i, j;
//    float x, fac, k, tmp;

//    /* Calculate initial values based on sine curve */
//    fac = 2.0 * PI;
//    k = 0.0; 
//    tmp = tpoints - 1;
//    for (j = 1; j <= tpoints; j++) {
//       x = k/tmp;
//       values[j] = sin (fac * x);
//       k = k + 1.0;
//    } 

//    /* Initialize old values array */
//    for (i = 1; i <= tpoints; i++) 
//       oldval[i] = values[i];
// }

// /**********************************************************************
//  *      Calculate new values using wave equation
//  *********************************************************************/
// void do_math(int i)
// {
//    float dtime, c, dx, tau, sqtau;

//    dtime = 0.3;
//    c = 1.0;
//    dx = 1.0;
//    tau = (c * dtime / dx);
//    sqtau = tau * tau;
//    newval[i] = (2.0 * values[i]) - oldval[i] + (sqtau *  (-2.0)*values[i]);
// }

/**********************************************************************
 *     Update all values along line a specified number of times
 *********************************************************************/
__global__ void update_parallel(float *values_D, int tpoints, int nsteps)
{
   // init line
   int i;
   float x, fac, tmp;
   int k;
   k = blockIdx.x * blockDim.x + (threadIdx.x + 1); // *value = 1 base
   /* Calculate initial values based on sine curve */
   float old_tmp, new_tmp, val_tmp;
   fac = 2.0 * PI;
   tmp = tpoints - 1;
  
   if (k <= tpoints){   
      //x = k/tmp;
      x = (k-1) / tmp;
      val_tmp = sin (fac * x);
      old_tmp = val_tmp;

      float dtime, c, dx, tau, sqtau;
      dtime = 0.3;
      c = 1.0;
      dx = 1.0;
      tau = (c * dtime / dx);
      sqtau = tau * tau;
  
      /* Update values for each time step */
      for (i = 1; i<= nsteps; i++) {
         /* Update points along line for this time step */
            /* global endpoints */
         if ((k == 1) || (k  == tpoints))
            new_tmp = 0.0;
         else
            // do math 
            new_tmp = (2.0 * val_tmp) - old_tmp + (sqtau * (-2.0) * val_tmp);
         
         /* Update old values with new values */
         old_tmp = val_tmp;
         val_tmp = new_tmp;
      }

      // read back from device
      values_D[k] = val_tmp;
   }
}


/**********************************************************************
 *     Print final results
 *********************************************************************/
void printfinal()
{
   int i;

   for (i = 1; i <= tpoints; i++) {
      printf("%6.4f ", values[i]);
      if (i%10 == 0)
         printf("\n");
   }
}

/**********************************************************************
 *	Main program
 *********************************************************************/
int main(int argc, char *argv[])
{
	sscanf(argv[1],"%d",&tpoints);
	sscanf(argv[2],"%d",&nsteps);
	check_param();

   // define cuda parameters
   float *values_D, *oldval_D, *newval_D;
   int size = (tpoints+2) * sizeof(float);
   cudaMalloc((void**)&values_D, size);
   //cudaMalloc((void**)$oldval_D, size);
   //cudaMalloc((void**)$newval_D, size);
   //cudaMemcpy(values_D, values, size, cudaMemcpyHostToDevice);
   //cudaMemcpy(oldval_D, oldval, size, cudaMemcpyHostToDevice);
   //cudaMemcpy(newval_D, newval, size, cudaMemcpyHostToDevice);

   // dimension = 1D 
   int threadsPerBlock = 1024;
   int numBlocks = ((tpoints%threadsPerBlock) == 0)? \
                     (tpoints/threadsPerBlock) : (tpoints/threadsPerBlock) + 1;

	printf("Initializing points on the line...\n");
	//init_line();
	printf("Updating all points for all time steps...\n");
	//update();
	update_parallel <<<numBlocks, threadsPerBlock>>> (values_D, tpoints, nsteps);
   cudaMemcpy(values, values_D, size, cudaMemcpyDeviceToHost);

   printf("Printing final results...\n");
	printfinal();
	printf("\nDone.\n\n");
	
   cudaFree(values_D);
	return 0;
}