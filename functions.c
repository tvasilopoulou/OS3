#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <time.h>
#include <wait.h>
#include <stdint.h>
#include <sys/sem.h>
#include <string.h>
#include <sys/errno.h>
#include <math.h>
#include "functions.h"
#include "md5.h"



int sharedMemoryCreate(int key){			//create shared memory segment with shmget
	int shmid = shmget(key, sizeof(shm *), IPC_CREAT|0600);
	if(shmid<0){
		printf("Error while creating a shared memory segment.\n");
		perror("Semcreate\n");
		printf("%d", errno);
		exit(1);
	}
	return shmid;
}

shm * sharedMemoryAttach(int key){			//attach key to shared memory segment with shmat
	shm * shMem;
	shMem= (shm *)shmat(key, NULL, 0); 	//DEFINITE NEED FOR CASTING
	if(shMem==(shm *)-1){
		printf("Error while attaching to a shared memory segment.\n");
		exit(-1);
	}
	return shMem;
}




int semaphoreCreate(int key, int value){			//create and initialize semaphore
	int sem, val;
	sem=semget(key,1,IPC_CREAT|0600);
	if(sem==-1){
		printf("Error while creating semaphore.\n");
		exit(-1);
	}
	union semun{			//for the use of semctl, linux manual
		int val;
		struct semid_ds * buf;
		ushort * array;
	}arg;
	arg.val=value;
	//now we initialise
	int s=semctl(sem, 0, SETVAL, arg.val);
	if(s==-1){
		printf("Error while initialising the new semaphore.\n");
		exit(-1);
	}
	return sem;
}

int down(int * sem){			//decrement semaphore
	struct sembuf down;
	down.sem_num=0;
	down.sem_op=-1;
	down.sem_flg=0;
	if(semop(*sem, &down, 1)==-1){
		printf("Error while reducing semaphore value.\n");
		exit(-1);
	}
	return 0;
}

int up(int * sem){				//increment semaphore
	struct sembuf up;//=(struct sembuf)malloc(sizeof(struct sembuf));
	up.sem_num=0;
	up.sem_op=1;
	up.sem_flg=0;
	if(semop(*sem, &up, 1)==-1){
		printf("Error while increasing semaphore value.\n");
		exit(-1);
	}
	return 0;
}



char * concat(char *s1, char *s2){			//https://stackoverflow.com/questions/8465006/how-do-i-concatenate-two-strings-in-c
    const size_t len1 = strlen(s1);
    const size_t len2 = strlen(s2);
    char * result = malloc(len1 + len2 + 1); 		//stick s2 in the end of s1 (like strncat but without causing a segmentation fault)
    if(!result){
		printf("Something related to memory allocation went wrong.\n");
		exit(-1);
    }
    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2 + 1);
    return result;
}


char * getRandomFileLine(){
	FILE * textFile=fopen("text.txt", "r");		//open logfile to write
	if (!textFile){
		printf("Unable to open text file.\n");
		exit(-1);
	}
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	int counter=rand()%countDocLines(textFile) +1;			//get random number of line from text file
	rewind(textFile);
	int count=counter;
	while((read = getline(&line, &len, textFile)) != -1 && count!=1) count--;		//read lines until i reach the one i want
	fclose(textFile);
	return line;
}



/*----------------------------------------------------------------------------------------------------------------------------------*/

void *producer(void *param) {
	int flag=0;
	while(1) {
		if((sm->numOfItems)<=0){			//once all items are created, signal to cancel producers thread
			pthread_cancel(ptid);
		} 	
		if(down(&(sm->semEmpty))<0){		//access shared memory segment, shared memory no longer empty
			printf("Error while decrementing semaphore empty.\n");
			exit(1);
		}
		pthread_mutex_lock(&mutex);			//acquire control by locking mutex
		printf("Current number of items to be produced: %d\n",sm->numOfItems );

		//critical section starts

		char * line=getRandomFileLine();		//line to be transmitted
		strncpy(sm->inText, line, strlen(line)+1);		//copy to shared memory
		sm->pid=pthread_self();					//send producer thread along
		(stats->steps ++);
		if(strcmp(sm->outText, "\0")){			// if message from consumer received
			flag=1;
			char * messagePid=strtok(sm->outText, "+");		//break apart tid from md5 string 
			char * md5string=strtok(NULL, "+");
			if(pthread_self() == atol(messagePid)){			//if current producer was the one who made the original product
				stats->pidMatch=1;
				stats->pidMatchSum++;						//number of same producer-receiver threads
				stats->pidMatch=0;				//to demonstrate change in pid_match 
				for(int i=0; i<strlen(sm->outText); i++) sm->outText[i]='\0';	//if sm->outText not empty, final move would be to empty it
				printf("Producer: self (%ld) -> ", pthread_self());
			}
			else{
				printf("Producer: thread %ld (self is %ld) -> ", atol(messagePid), pthread_self());
			}
			printf("MD5 conversion value: [%s], item no.%d\n", md5string, (sm->numOfItems)+1);
			(stats->steps ++);
		}


		//critical section ends

		pthread_mutex_unlock(&mutex);
		if(up(&(sm->semFull))<0){
			printf("Error while incrementing semaphore full.\n");
			exit(1);
		}

	}
	if(flag!=1){			//if consumer not responding
		printf("Error: Consumer not responding. Producer exiting with error code...");
		exit(-1);
	}
	return NULL;
}

void *consumer(void *param) {
	while(1) {
		if((sm->numOfItems)<=0){		//if no more items to consume, cancel consumer thread
			pthread_cancel(ctid);
		} 
		if(down(&(sm->semFull))<0){		//decrement full semaphore
			printf("Error while decrementing semaphore full.\n");
			exit(1);
		}		
		pthread_mutex_lock(&mutex);
		printf("Current number of items to be consumed: %d\n",sm->numOfItems );

		//critical section starts

 		//md5 implementation: http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
		unsigned char digest[16];									//create md5 encoding
 		
		MD5_CTX context;
		MD5_Init(&context);
		// MD5_Update(&context, string, strlen(string));
		MD5_Update(&context, sm->inText, strlen(sm->inText));
		MD5_Final(digest, &context);
		char md5string[33];
		for(int i=0; i<16;i++) sprintf(&md5string[i*2], "%02x", (unsigned int)digest[i]);
		for(int i=0; i<strlen(sm->inText); i++)sm->inText[i]='\0';

		
		sprintf(message, "%ld", sm->pid);		//copy producer id to message
    	message=concat(message, "+");			//and concatenate it with line md5 hash value
    	message=concat(message, md5string);
		strncpy(sm->outText, message, strlen(message)+1);			//copy that string to send
		for(int i=0; i<strlen(message); i++) message[i]='\0';		//empty message variable
		(stats->steps ++);
		(sm->numOfItems)--;			//item consumed
		//critical section ends

		//DO STUFF
		pthread_mutex_unlock(&mutex);
		if(up(&(sm->semEmpty))<0){
			printf("Error while incrementing semaphore empty.\n");
			exit(1);
		}
	}
}

void setSignalHandler(void){					//not required, set for signals
	static struct sigaction act;				//in case of interrupt or quit
	act.sa_handler = catchInterrupt ;			//call handler
	sigfillset(&( act.sa_mask )) ;
	sigaction( SIGSEGV , &act , NULL ) ;			//signals for interrupts
}


void catchInterrupt ( int signo ) {				//not required, set for signals
	printf ( "\nCatching : signo =% d \n" , signo ) ;
	exit(-1);
}

void initStats(int numOfP){						//initialize statistics struct
	stats=malloc(sizeof(statistics));
	stats->numOfP=numOfP;			//make these segments into functions!!!
	stats->steps=0;
	stats->pidMatch=0;
	stats->pidMatchSum=0;
}

int initSM(int smkey, int numOfItems){			//initaliaze shared memory segment data
	smkey=sharedMemoryCreate((SHMKEY+0));
	sm=sharedMemoryAttach(smkey);
	sm->semFull=semaphoreCreate((SEMKEY+1), 0);
	sm->semEmpty=semaphoreCreate((SEMKEY+2), 1);
	sm->numOfItems=numOfItems;
	sm->inText=malloc(sizeof(char)*180);		//max num of CPL allowed in programming
	sm->outText=malloc(sizeof(char)*49);		//32-byte md5string + 15-byte hash + '\0' + '+'
	for(int i=0; i<strlen(sm->outText); i++) sm->outText[i]='\0';
	return smkey;
}

void presentStats(void){
	printf("\n Statistics Board: \n");
	printf("------------------\n");
	printf("a. Number of Producers: %d\n", stats-> numOfP);
	printf("b. Number of Steps: %d\n", stats->steps );
	printf("c. Number of matching id of Producer-Receiver: %d\n", stats->pidMatchSum);
}

int countDocLines(FILE * file){
	int lines=0;
	char c;
	while(!feof(file)){		//until end of file
		c=fgetc(file);		//read char
		if (c=='\n') lines++;		//if char is next line increment counter
	}
	return lines;
}
