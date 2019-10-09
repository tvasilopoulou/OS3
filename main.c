#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <time.h>
#include <wait.h>
#include <sys/sem.h>
#include <string.h>
#include <pthread.h>
#include "functions.h"

/*	./main 10 60		*/

int main(int argc, char * argv[]){
	if (argc!=3){
		printf("Error: This program can only accept 3 arguments: [./prog_name N K]. Please try again.\n");
		exit(-1);
	}
	int numOfP=atoi(argv[1]);				//number of producers
	int numOfItems=atoi(argv[2]);			//number of components

  /*----------------------------------------------------------------------------------------------------------------------------------*/
	// setSignalHandler();			//in case SIGSETV should  have been set, already implemented
	printf("Welcome to Producer(s)-Consumer Line. We have been asked to produce %d items. Is that correct? (y/n) ", numOfItems);
	char answer[1];					//verify number of items
	scanf("%s", answer);
	if(!strcmp(answer, "n")){
		printf("We are sorry for the inconvenience. Please enter a number:(0-999) ");
		scanf("%s", answer);		//recalculate
		numOfItems=atoi(answer);
	}
	printf("Okay, we will produce %d items for you. Please hold.\n", numOfItems);
	message=malloc(sizeof(char)*16);	//for shared memory segment, allocate once
	sleep(1); //so that the message is seen, can be omitted
  /*----------------------------------------------------------------------------------------------------------------------------------*/
	initStats(numOfP);			//for statistics calculations
  /*----------------------------------------------------------------------------------------------------------------------------------*/
	int smkey=initSM(smkey, numOfItems);	//creation of shared memory segment
  /*----------------------------------------------------------------------------------------------------------------------------------*/
	/*threads creation*/
	for(int i = 0; i < numOfP; i++){		//create numOfP producers
		if(pthread_create(&ptid,NULL,producer,NULL)){
			perror("Producer");
			exit(-1);
		}	
	} 
	sleep(10);			//allow producer threads to run
	if(pthread_create(&ctid, NULL,consumer, NULL)){		//create one consumer
      perror("Consumer");
      exit(-1);
    }
    sleep(10);			//allow consumer thread to run
  /*----------------------------------------------------------------------------------------------------------------------------------*/

	if(shmdt(sm)<0){									//detach shared memory segments
		perror("Error while detaching shared memory segment");
		exit(-1);
	}

	if(shmctl(smkey, IPC_RMID, (struct shmid_ds *) NULL)<0){	//delete
		perror("Error while deleting a shared memory segment");
		exit(-1);
	}
	presentStats();
	free(message);
	free(stats);
	printf("Exiting Producer(s)-Consumer Line. We hope to see you again soon. Bye bye!\n");
	exit(0);
}