#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>
#include <wait.h>
#include <sys/sem.h>
#include <stdbool.h>

#define SHMKEY 1111
#define SEMKEY 9999


pthread_t ptid,ctid;
pthread_mutex_t mutex;



typedef struct sharedMemory{
    int semFull;            //full semaphore
    int semEmpty;           //empty semaphore
    int numOfItems;
    char * inText;
    char * outText;
    pthread_t pid;
}shm;

shm * sm;

typedef struct statistics{
    int numOfP;
    int steps;
    int pidMatch;
    int pidMatchSum;
}statistics;

statistics * stats;
char * message;


int sharedMemoryCreate(int);
shm * sharedMemoryAttach(int);
void sharedMemoryDetach(shm * , int );
int semaphoreCreate(int , int );
int down(int *);
int up(int * );
char * concat(char * , char *);
void *producer(void * );
void *consumer(void *);
void setSignalHandler(void);
void catchInterrupt ( int  );
void initStats(int);
int initSM(int, int);
char * getRandomFileLine(void);
void presentStats(void);
int countDocLines(FILE * );