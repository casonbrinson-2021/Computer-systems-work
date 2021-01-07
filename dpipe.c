// Created by: Gregory  Cason Brinson
// last edited: 2/7/20
// Honor code: " I did not give or recieve any unauthorized help on this assignment."

#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>



int main(int argc, char **argv)
{
	//defining the pid_t names os we can use them throughout the program
	pid_t child1, child2;
	//create the two pipes read and write ends
	int fd1[2];
	int fd2[2];

	pipe(fd1);
	pipe(fd2);

	//fork to create the first child (child1)
	child1 = fork();

	//this only runs when child1 is the process running
	if(child1 == 0)
	{
		//thes are all the system calls that my program used in order
		// for child1 (wc child)
		char *argc1[] = {argv[1], NULL};
		printf("Starting: %s\n", argc1[0]);
		dup2(fd1[1], 1);
		close(fd1[1]);
		dup2(fd2[0], 0);
		close(fd2[1]);
		close(fd2[0]);
		close(fd1[0]);
		execvp(argc1[0], argc1);
	}
	else
	{
		//creating the second child
		child2 = fork();

		//this only runs when child2 is the process running
		if(child2 == 0)
		{
			//thes are all the system calls that wer uses
			// for child2 (gnetcat child)
			char **argc2 = argv;
			argc2 = argc2 + 2;
			printf("Starting: %s\n", argc2[0]);
			dup2(fd1[0],0);
			close(fd2[0]);
			dup2(fd2[1],1);
			close(fd2[1]);
			close(fd1[0]);
			close(fd1[1]);
			execvp(argc2[0], argc2);
		}
		
		//this only runs when its the parent running
		else 
		{
			//thse are the parents system call after
			//it creates its two children
			close(fd2[1]);	
			close(fd2[0]);
			close(fd1[1]);
			close(fd1[0]);
			wait(NULL);
			wait(NULL);
		}
	}



	return 0;
}
