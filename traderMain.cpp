#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include "trader.h"

#define NUM_THREADS 10
pthread_t *thread;
pthread_mutex_t consolemutex, workermutex;
int thread_count = 0;

int main(int argc, char **argv){
    int retcode = 0;
    int num_instances;
    tradeParams **tradeParams;
    FILE *datafile = NULL, *tradesFile;
    char buffer[100];
    void *status;
    const char *tradeSequenceFile = "tradesFile.csv";
    
    if (argc != 2){
        printf("usage: trader filename\n");  retcode = 1;
	    goto BACK;
    }
    pthread_mutex_init(&consolemutex, NULL);
    pthread_mutex_init(&workermutex, NULL);
    datafile = fopen(argv[1], "r");
    if(!datafile){
        printf("cannot open file %s\n", argv[1]);
        retcode = 2; goto BACK;
    }
    printf("reading datafile %s\n", argv[1]);
    fscanf(datafile, "%s", buffer);
	fscanf(datafile, "%s", buffer);
    num_instances = atoi(buffer);
    tradeParams = (struct tradeParams **)calloc(num_instances, sizeof(struct tradeParams *));
    printf("%d\n", num_instances);
    
    for (int i = 0; i < num_instances; i ++){
        tradeParams[i] = (struct tradeParams *)calloc(1, sizeof(struct tradeParams));
        fscanf(datafile, "%s", buffer);
        tradeParams[i]->num_shares = atoi(buffer);
        fscanf(datafile, "%s", buffer);
        tradeParams[i]->num_days = atoi(buffer);
        fscanf(datafile, "%s", buffer);
        tradeParams[i]->alpha = atof(buffer);
        fscanf(datafile, "%s", buffer);
        tradeParams[i]->Pi_1 = atof(buffer);
        fscanf(datafile, "%s", buffer);
        tradeParams[i]->Pi_2 = atof(buffer);
        fscanf(datafile, "%s", buffer);
        tradeParams[i]->p1 = atof(buffer);
        fscanf(datafile, "%s", buffer);
        tradeParams[i]->p2 = atof(buffer);
        fscanf(datafile, "%s", buffer);
        tradeParams[i]->rho = atof(buffer);
    }
    thread = (pthread_t *)calloc(num_instances, sizeof(pthread_t));
    for (int i = 0; i < num_instances; i ++){
        if(pthread_create(&thread[i], NULL, execute, (void *)tradeParams[i])){
            printf("error creating thread %d\n", i);
        }
    }

    for (int i = 0; i < num_instances; i ++){
        if(pthread_join(thread[i], &status)) {
            printf("error joining thread %d\n", i);
        }
    }
    tradesFile = fopen(tradeSequenceFile, "w");
    for (int i = 0; i < num_instances; i++){
        printf("optimal value for trade sequencing for instance %d is %f\n", i, tradeParams[i]->optimal[tradeParams[i]->num_shares]);
        printf("optimal sequence of shares to announce\n");
        for(int j = 0; j < tradeParams[i]->solvec.size(); j ++){
            std::cout << tradeParams[i]->solvec[j] << " ";
            fprintf(tradesFile, "%d,", tradeParams[i]->solvec[j]);
        }
        printf("\n");
        fprintf(tradesFile, "\n");
    }
    fclose(tradesFile);

    BACK:
    pthread_mutex_destroy(&consolemutex);
    pthread_exit(NULL);
    return retcode;
}

void *execute(void *mytradeParams){
    int flag = 0;
    while(flag == 0){
        usleep(100);
        pthread_mutex_lock(&workermutex);
        if (thread_count < NUM_THREADS){
            thread_count += 1;
            flag = 1;
        }
        pthread_mutex_unlock(&workermutex);
    }
    
    pthread_t tid = pthread_self();
    tradeParams *tradeParams = (struct tradeParams*)mytradeParams;
    int retcode = 0;
    double *shift, *optimal, bestone, candidate;
    int N, T, temp_h;
    int *sol;
    double alpha, pi_1, pi_2, p1, p2, rho;

    N = tradeParams->num_shares;
    T = tradeParams->num_days;
    alpha = tradeParams->alpha;
    pi_1 = tradeParams->Pi_1;
    pi_2 = tradeParams->Pi_2;
    p1 = tradeParams->p1;
    p2 = tradeParams->p2;
    rho = tradeParams->rho;
    shift = (double *)calloc(N + 1, sizeof(double));
    tradeParams->optimal = (double *)calloc((N + 1)*T, sizeof(double));
    sol = (int *)calloc((N + 1)*T, sizeof(int));

    /** do last stage **/
    for (int j = 0; j <= N; j++){
        shift[j] = p1*(1 - alpha*pow((double)j, pi_1)) + p2*(1 - alpha*pow((double)j, pi_2));
        tradeParams->optimal[(T - 1)*(N + 1) + j] = shift[j]*j;
        sol[(T - 1)*(N + 1) + j] = j;
    }

    for (int t = T - 2; t >= 0; t --){
	    for (int j = 0; j <= N; j ++){

		    bestone = 0;
            temp_h = 0;
		    /** enumerate possibilities **/
		    for (int h = 0; h <= j; h++){
			    //newprice = shift[h];
			    candidate = shift[h]*((1-rho)*tradeParams->optimal[(t+1)*(N + 1) + j] + rho*(h + tradeParams->optimal[(t + 1)*(N + 1) + j - h]));
			    if (candidate > bestone){
                    bestone = candidate;
                    temp_h = h;
                }
			    	
		    }
		    tradeParams->optimal[t*(N + 1) + j] = bestone;
            sol[t*(N + 1) + j] = temp_h;
            /*
            if(t == 0 && j == N){
                pthread_mutex_lock(&consolemutex);
                printf ("%f   %d", bestone, temp_h);
                pthread_mutex_unlock(&consolemutex);
            }
            */
        }
        //pthread_mutex_lock(&consolemutex);
        //printf("done with stage t = %d for thread %p\n", t, tid);
        //pthread_mutex_unlock(&consolemutex);
    }
    int index = N;

	for (int k = 0; k < T; k++)
	{	
		
		tradeParams->solvec.push_back(sol[k * (N + 1) + index]);
        //std::cout << index << std::endl;
        index = index - sol[k * (N + 1) + index];
    }

    pthread_mutex_lock(&consolemutex);
    thread_count -= 1;
    printf("Done with thread %p \n", tid);
    pthread_mutex_unlock(&consolemutex);

    return NULL;
    
}