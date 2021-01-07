/*	Created by: Gregory Cason Brinson
 *	Date: 3/23/20
 * 	Filename: outfoxed.c
 */

#include "outfoxed.h"
 
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <sched.h>
#include <semaphore.h>

#define NUMBER_OF_RED_CLUES 3
#define NUMBER_OF_GREEN_CLUES 9


/* condition varaible and mutex declarations */
static pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t analyze_data = PTHREAD_COND_INITIALIZER;
static pthread_cond_t get_data = PTHREAD_COND_INITIALIZER;

static bool newDataAvailable;	//starts off as false
static bool needData;			//starts off as true
static bool thiefCaught;		//starts off as false

/* Semaphore declarations */
sem_t glasses_done;
sem_t hat_done;
sem_t necklace_done;
sem_t flower_done;
sem_t watch_done;
sem_t scarf_done;
sem_t coat_done;
sem_t monocle_done;
sem_t gloves_done;
sem_t briefcase_done;
sem_t cane_done;

/* storage for suspects revealed and clues revealed */
int revSus[NUMBER_OF_SUSPECTS];
enum Clue revealedRedClues[NUMBER_OF_RED_CLUES];
enum Clue revealedGreenClues[NUMBER_OF_GREEN_CLUES];
int suspectIndex, currentGreenIndex;
int eliminatedCount;




/*	function definitions */

//simply the logic that compares clues revealed to each suspect that has been revealed and eliminates the suspects that don't have that clue etc.
//should be sleeping or waiting (blocked state) until a clue or suspect arrives
extern void * chicken_detective_thread(void *p)
{	
	//keeps it looping
	while(1)
	{
		//lock
		pthread_mutex_lock(&mlock);
		//wait to do anything until new data is available or until the thief has been caught
		
		//////////this is the problem//////////////////////
		while(!newDataAvailable && !thiefCaught)
			pthread_cond_wait(&analyze_data, &mlock);
		///////////////////////////////////////////////////
		
		//if the thief has been caught then return
		if(thiefCaught)
		{
			//broadcast to unlock all other chickens
			pthread_cond_broadcast(&analyze_data);
			//unlock
			pthread_mutex_unlock(&mlock);
			//return this chicken
			return p;
		}
		
		//if theres still suspects to eliminate
		if(eliminatedCount < 15)
		{
			//logic to eliminate suspects
			for(int i = 0; i < NUMBER_OF_SUSPECTS; i++)
			{
				if(revSus[i] != -1)
				{
					for(int j = 0; j < NUMBER_OF_GREEN_CLUES; j++)
					{
						if(revSus[i] != -1)
						{
							if((suspects[revSus[i]].clues[0] == revealedGreenClues[j]) || (suspects[revSus[i]].clues[1] == revealedGreenClues[j]) || (suspects[revSus[i]].clues[2] == revealedGreenClues[j]))
							{
								eliminate_suspect(&suspects[revSus[i]]);
								eliminatedCount++;
								revSus[i] = -1;
							}
						}			
					}
				}
			}
			if(eliminatedCount < 15)
			{
				needData = true;
				newDataAvailable = false;
				//signal for the next clue things to stop waiting
				pthread_cond_signal(&get_data);
			}
			else needData = false;
		}
	
		//if we only have one suspect left
		else
		{
			//try to catch the thief
			for(int i = 0; i < NUMBER_OF_SUSPECTS; i++)
			{
				if(revSus[i] != -1)
				{
					//set the game over condition
					thiefCaught = true;
					//broadcast to unlock all other chickens
					pthread_cond_broadcast(&analyze_data);
					//broadcast to unlock all other clue threads
					pthread_cond_broadcast(&get_data);
					//annoucne the thief correctly
					announce_thief(&suspects[revSus[i]]);
					//unlock
					pthread_mutex_unlock(&mlock);
					//return
					//printf("the chicken that found the thief is returning*************8\n");
					return p;
				}
			}
		}
		//unlock
		pthread_mutex_unlock(&mlock);
	}
}

//this function is given a suspect that has been revealed. All you do is let the chicken detective know which suspect has been revealed so that it can check it against the currently revealed clues LOL!
extern void * new_suspect_thread(void *suspect)
{
	//lock
	pthread_mutex_lock(&mlock);
	pthread_cond_signal(&analyze_data);
	
	Suspect *theSuspect =  suspect;
	revSus[suspectIndex] = theSuspect->number;
	suspectIndex++;
	
	newDataAvailable = true;
	
	pthread_cond_signal(&analyze_data);
	
	//unlock
	pthread_mutex_unlock(&mlock);
	
	return suspect;
}

//These are all threads that simply decode clues specific to them
//need to remove the print statements in here
//need to add...
//lock things and then unlock them and send signal to chickens to continue analyzing
extern void * glasses_thread(void *p)
{
	//lock
	pthread_mutex_lock(&mlock);
	enum Clue clue = glasses;
	enum ClueColor cc = decode_clue(clue);
	if(cc == GREEN)
	{
		revealedGreenClues[currentGreenIndex] = glasses;
		currentGreenIndex++;
		newDataAvailable = true;
	}
	else newDataAvailable = false;
	sem_post(&glasses_done);
	pthread_cond_signal(&analyze_data);
	//unlock
	pthread_mutex_unlock(&mlock);
	
	
	return p;
}
extern void * hat_thread(void *p)
{
	//wait for the previous clue to be done
	sem_wait(&glasses_done);
	//lock
	pthread_mutex_lock(&mlock);
	//while their is no need for new data and the thief has not been caught wait
	while(!needData && !thiefCaught)
		pthread_cond_wait(&get_data, &mlock);
	//check to see if the thief was caught
	if(thiefCaught)
	{
		newDataAvailable = false;
		//allow the next clue thing to get unstuck
		sem_post(&hat_done);
		//allow any waiting chickens to move
		pthread_cond_broadcast(&analyze_data);
		//unlock
		pthread_mutex_unlock(&mlock);
		//return
		return p;
	}
	
	enum Clue clue = hat;
	enum ClueColor cc = decode_clue(clue);
	if(cc == GREEN)
	{
		revealedGreenClues[currentGreenIndex] = hat;
		currentGreenIndex++;
		newDataAvailable = true;
	}
	sem_post(&hat_done);
	pthread_cond_signal(&analyze_data);
	//unlock
	pthread_mutex_unlock(&mlock);
	
	return p;
}
extern void * necklace_thread(void *p)
{
	//wait for the previous clue to be done
	sem_wait(&hat_done);
	//lock
	pthread_mutex_lock(&mlock);
	//while their is no need for new data and the thief has not been caught wait
	while(!needData && !thiefCaught)
		pthread_cond_wait(&get_data, &mlock);
	//check to see if the thief was caught
	if(thiefCaught)
	{
		newDataAvailable = false;
		//allow the next clue thing to get unstuck
		sem_post(&necklace_done);
		//allow any waiting chickens to move
		pthread_cond_broadcast(&analyze_data);
		//unlock
		pthread_mutex_unlock(&mlock);
		//return
		return p;
	}
	enum Clue clue = necklace;
	enum ClueColor cc = decode_clue(clue);
	if(cc == GREEN)
	{
		revealedGreenClues[currentGreenIndex] = necklace;
		currentGreenIndex++;
		newDataAvailable = true;
	}
	sem_post(&necklace_done);
	pthread_cond_signal(&analyze_data);
	//unlock
	pthread_mutex_unlock(&mlock);
	
	return p;
}
extern void * flower_thread(void *p)
{
	//wait for the previous clue to be done
	sem_wait(&necklace_done);
	//lock
	pthread_mutex_lock(&mlock);
	//while their is no need for new data and the thief has not been caught wait
	while(!needData && !thiefCaught)
		pthread_cond_wait(&get_data, &mlock);
	//check to see if the thief was caught
	if(thiefCaught)
	{
		newDataAvailable = false;
		//allow the next clue thing to get unstuck
		sem_post(&flower_done);
		//allow any waiting chickens to move
		pthread_cond_broadcast(&analyze_data);
		//unlock
		pthread_mutex_unlock(&mlock);
		//return
		return p;
	}
	enum Clue clue = flower;
	enum ClueColor cc = decode_clue(clue);
	if(cc == GREEN)
	{
		revealedGreenClues[currentGreenIndex] = flower;
		currentGreenIndex++;
		newDataAvailable = true;
	}
	sem_post(&flower_done);
	pthread_cond_signal(&analyze_data);
	//unlock
	pthread_mutex_unlock(&mlock);
	
	return p;
}
extern void * watch_thread(void *p)
{	
	//wait for the previous clue to be done
	sem_wait(&flower_done);
	//lock
	pthread_mutex_lock(&mlock);
	//while their is no need for new data and the thief has not been caught wait
	while(!needData && !thiefCaught)
		pthread_cond_wait(&get_data, &mlock);
	//check to see if the thief was caught
	if(thiefCaught)
	{
		newDataAvailable = false;
		//allow the next clue thing to get unstuck
		sem_post(&watch_done);
		//allow any waiting chickens to move
		pthread_cond_broadcast(&analyze_data);
		//unlock
		pthread_mutex_unlock(&mlock);
		//return
		return p;
	}
	enum Clue clue = watch;
	enum ClueColor cc = decode_clue(clue);
	if(cc == GREEN)
	{
		revealedGreenClues[currentGreenIndex] = watch;
		currentGreenIndex++;
		newDataAvailable = true;
	}
	sem_post(&watch_done);
	pthread_cond_signal(&analyze_data);
	pthread_mutex_unlock(&mlock);
	
	return p;
}
extern void * scarf_thread(void *p)
{
	//wait for the previous clue to be done
	sem_wait(&watch_done);
	//lock
	pthread_mutex_lock(&mlock);
	//while their is no need for new data and the thief has not been caught wait
	while(!needData && !thiefCaught)
		pthread_cond_wait(&get_data, &mlock);
	//check to see if the thief was caught
	if(thiefCaught)
	{
		newDataAvailable = false;
		//allow the next clue thing to get unstuck
		sem_post(&scarf_done);
		//allow any waiting chickens to move
		pthread_cond_broadcast(&analyze_data);
		//unlock
		pthread_mutex_unlock(&mlock);
		//return
		return p;
	}
	enum Clue clue = scarf;
	enum ClueColor cc = decode_clue(clue);
	if(cc == GREEN)
	{
		revealedGreenClues[currentGreenIndex] = scarf;
		currentGreenIndex++;
		newDataAvailable = true;
	}
	sem_post(&scarf_done);
	pthread_cond_signal(&analyze_data);
	//unlock
	pthread_mutex_unlock(&mlock);
	
	return p;
}
extern void * coat_thread(void *p)
{
	//wait for the previous clue to be done
	sem_wait(&scarf_done);
	//lock
	pthread_mutex_lock(&mlock);
	//while their is no need for new data and the thief has not been caught wait
	while(!needData && !thiefCaught)
		pthread_cond_wait(&get_data, &mlock);
	//check to see if the thief was caught
	if(thiefCaught)
	{
		newDataAvailable = false;
		//allow the next clue thing to get unstuck
		sem_post(&coat_done);
		//allow any waiting chickens to move
		pthread_cond_broadcast(&analyze_data);
		//unlock
		pthread_mutex_unlock(&mlock);
		//return
		return p;
	}
	enum Clue clue = coat;
	enum ClueColor cc = decode_clue(clue);
	if(cc == GREEN)
	{
		revealedGreenClues[currentGreenIndex] = coat;
		currentGreenIndex++;
		newDataAvailable = true;
	}
	sem_post(&coat_done);
	pthread_cond_signal(&analyze_data);
	//unlock
	pthread_mutex_unlock(&mlock);
	
	return p;
}
extern void * monocle_thread(void *p)
{
	//wait for the previous clue to be done
	sem_wait(&coat_done);
	//lock
	pthread_mutex_lock(&mlock);
	//while their is no need for new data and the thief has not been caught wait
	while(!needData && !thiefCaught)
		pthread_cond_wait(&get_data, &mlock);
	//check to see if the thief was caught
	if(thiefCaught)
	{
		newDataAvailable = false;
		//allow the next clue thing to get unstuck
		sem_post(&monocle_done);
		//allow any waiting chickens to move
		pthread_cond_broadcast(&analyze_data);
		//unlock
		pthread_mutex_unlock(&mlock);
		//return
		return p;
	}
	enum Clue clue = monocle;
	enum ClueColor cc = decode_clue(clue);
	if(cc == GREEN)
	{
		revealedGreenClues[currentGreenIndex] = monocle;
		currentGreenIndex++;
		newDataAvailable = true;
	}
	sem_post(&monocle_done);
	pthread_cond_signal(&analyze_data);
	//unlock
	pthread_mutex_unlock(&mlock);
	
	return p;
}
extern void * gloves_thread(void *p)
{
	//wait for the previous clue to be done
	sem_wait(&monocle_done);
	//lock
	pthread_mutex_lock(&mlock);
	//while their is no need for new data and the thief has not been caught wait
	while(!needData && !thiefCaught)
		pthread_cond_wait(&get_data, &mlock);
	//check to see if the thief was caught
	if(thiefCaught)
	{
		newDataAvailable = false;
		//allow the next clue thing to get unstuck
		sem_post(&gloves_done);
		//allow any waiting chickens to move
		pthread_cond_broadcast(&analyze_data);
		//unlock
		pthread_mutex_unlock(&mlock);
		//return
		return p;
	}
	enum Clue clue = gloves;
	enum ClueColor cc = decode_clue(clue);
	if(cc == GREEN)
	{
		revealedGreenClues[currentGreenIndex] = gloves;
		currentGreenIndex++;
		newDataAvailable = true;
	}
	sem_post(&gloves_done);
	pthread_cond_signal(&analyze_data);
	//unlock
	pthread_mutex_unlock(&mlock);
	
	return p;
}
extern void * briefcase_thread(void *p)
{
	//wait for the previous clue to be done
	sem_wait(&gloves_done);
	//lock
	pthread_mutex_lock(&mlock);
	//while their is no need for new data and the thief has not been caught wait
	while(!needData && !thiefCaught)
		pthread_cond_wait(&get_data, &mlock);
	//check to see if the thief was caught
	if(thiefCaught)
	{
		newDataAvailable = false;
		//allow the next clue thing to get unstuck
		sem_post(&briefcase_done);
		//allow any waiting chickens to move
		pthread_cond_broadcast(&analyze_data);
		//unlock
		pthread_mutex_unlock(&mlock);
		//return
		return p;
	}
	enum Clue clue = briefcase;
	enum ClueColor cc = decode_clue(clue);
	if(cc == GREEN)
	{
		revealedGreenClues[currentGreenIndex] = briefcase;
		currentGreenIndex++;
		newDataAvailable = true;
	}
	sem_post(&briefcase_done);
	pthread_cond_signal(&analyze_data);
	//unlock
	pthread_mutex_unlock(&mlock);
	
	return p;
}
extern void * cane_thread(void *p)
{
	//wait for the previous clue to be done
	sem_wait(&briefcase_done);
	//lock
	pthread_mutex_lock(&mlock);
	//while their is no need for new data and the thief has not been caught wait
	while(!needData && !thiefCaught)
		pthread_cond_wait(&get_data, &mlock);
	//check to see if the thief was caught
	if(thiefCaught)
	{
		newDataAvailable = false;
		//allow the next clue thing to get unstuck
		sem_post(&cane_done);
		//allow any waiting chickens to move
		pthread_cond_broadcast(&analyze_data);
		//unlock
		pthread_mutex_unlock(&mlock);
		//return
		return p;
	}
	enum Clue clue = cane;
	enum ClueColor cc = decode_clue(clue);
	if(cc == GREEN)
	{
		revealedGreenClues[currentGreenIndex] = cane;
		currentGreenIndex++;
		newDataAvailable = true;
	}
	sem_post(&cane_done);
	pthread_cond_signal(&analyze_data);
	//unlock
	pthread_mutex_unlock(&mlock);
	
	return p;
}
extern void * umbrella_thread(void *p)
{	
	//wait for the previous clue to be done
	sem_wait(&cane_done);
	//lock
	pthread_mutex_lock(&mlock);
	//while their is no need for new data and the thief has not been caught wait
	while(!needData && !thiefCaught)
		pthread_cond_wait(&get_data, &mlock);
	//check to see if the thief was caught
	if(thiefCaught)
	{
		newDataAvailable = false;
		//allow any waiting chickens to move
		pthread_cond_broadcast(&analyze_data);
		//unlock
		pthread_mutex_unlock(&mlock);
		//return
		return p;
	}
	enum Clue clue = umbrella;
	enum ClueColor cc = decode_clue(clue);
	if(cc == GREEN)
	{
		revealedGreenClues[currentGreenIndex] = umbrella;
		currentGreenIndex++;
		newDataAvailable = true;
	}
	pthread_cond_broadcast(&analyze_data);
	//unlock
	pthread_mutex_unlock(&mlock);
		
	return p;
}


//settup the semaphore values
extern void setup_play(void)
{
	
	sem_init(&glasses_done, 0, 0);
	sem_init(&hat_done, 0, 0);
	sem_init(&necklace_done, 0, 0);
	sem_init(&flower_done, 0, 0);
	sem_init(&watch_done, 0, 0);
	sem_init(&scarf_done, 0, 0);
	sem_init(&coat_done, 0, 0);
	sem_init(&monocle_done, 0, 0);
	sem_init(&gloves_done, 0, 0);
	sem_init(&briefcase_done, 0, 0);
	sem_init(&cane_done, 0, 0);
	
	thiefCaught = false;
	newDataAvailable = false;
	needData = true;
		
	eliminatedCount = 0;
	suspectIndex = 0;
	currentGreenIndex = 0;
	
	//initialized to -1 means that slot is "empty"
	revSus[0] = -1;
	revSus[1] = -1;
	revSus[2] = -1;
	revSus[3] = -1;
	revSus[4] = -1;
	revSus[5] = -1;
	revSus[6] = -1;
	revSus[7] = -1;
	revSus[8] = -1;
	revSus[9] = -1;
	revSus[10] = -1;
	revSus[11] = -1;
	revSus[12] = -1;
	revSus[13] = -1;
	revSus[14] = -1;
	revSus[15] = -1;
	
	revealedGreenClues[0] = -1;
	revealedGreenClues[1] = -1;
	revealedGreenClues[2] = -1;
	revealedGreenClues[3] = -1;
	revealedGreenClues[4] = -1;
	revealedGreenClues[5] = -1;
	revealedGreenClues[6] = -1;
	revealedGreenClues[7] = -1;
	revealedGreenClues[8] = -1;
}


