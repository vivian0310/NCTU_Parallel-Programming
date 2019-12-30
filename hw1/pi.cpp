#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <time.h>

pthread_mutex_t mutex;
double number_in_circle; 
unsigned long long number_of_cpu, number_of_tosses;

void *pi_calculate(void *th){
    double x, y;
    long my_thread = (long) th;
    long long toss;
    long long my_tossNum = number_of_tosses / number_of_cpu;
    long long tossFirst = my_tossNum * my_thread;
    long long tossLast = tossFirst + my_tossNum;
    double circle_count;

    srand(time(NULL));
    unsigned int seed = time(NULL);
    for (toss = tossFirst; toss < tossLast; toss++) {
        x = (double)rand_r(&seed) /RAND_MAX;
        y = (double)rand_r(&seed) /RAND_MAX;

        if (x*x + y*y <= 1.0)
            circle_count++;    
    }
    pthread_mutex_lock(&mutex);
    number_in_circle += circle_count;
    pthread_mutex_unlock(&mutex);
    return NULL;

}

int main(int argc, char **argv)
{
    double pi_estimate, cpu_time;
    unsigned long long toss, th;
    clock_t start, end;

    if (argc < 3) {
        exit(-1);
    }

    start = clock();
    number_of_cpu = atoi(argv[1]);
    number_of_tosses = atoi(argv[2]);
    if ((number_of_cpu < 1) || (number_of_tosses < 0)) {
        exit(-1);
    }
    
    pthread_t* threads_handle;
    threads_handle = (pthread_t*) malloc(number_of_cpu * sizeof(pthread_t));

    pthread_mutex_init(&mutex, NULL);
    number_in_circle = 0;

    for (th = 0; th < number_of_cpu; th++){
        int val = pthread_create(&threads_handle[th], NULL, pi_calculate, (void*)th);
        if (val){
            printf("fail to creat pthread: %d\n", val);
            exit(-1);
        }
    }

    for (th = 0; th < number_of_cpu; th++){
        pthread_join(threads_handle[th], NULL);
    }
    
    pthread_mutex_destroy(&mutex);
    free(threads_handle);
    pi_estimate = 4*number_in_circle/((double) number_of_tosses);
    end = clock();
    cpu_time = ((double) (end -start)) / CLOCKS_PER_SEC;
    printf("Pi = %lf\n",pi_estimate);
    printf("Executing time = %f sec\n", cpu_time);  
    
    return 0;
}
