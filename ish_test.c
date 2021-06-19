/*--------------------------------------------------------------------*/
/* dfa.c                                                              */
/* Original Author: Bob Dondero                                       */
/* Illustrate lexical analysis using a deterministic finite state     */
/* automaton (DFA)                                                    */
/*--------------------------------------------------------------------*/
#define _GNU_SOURCE
#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#include "dynarray.h"
#include "token.h"
#include "child.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/*--------------------------------------------------------------------*/

#define MAX_LINE_SIZE 1024
#define MAX_PATH_SIZE 1024
#define SYSTEM_NAME "./ish"

DynArray_T childPIDs;
DynArray_T tokens;
char *errMsg;
char **argv;
int number_token, number_argv, totalComm;
int *numArgv_each_Comm;

/* SIGCHLD_handler is to reap child process after they are exited and remove the process ID from child process ID list */
void SIGCHLD_handler(int iSig)
{
	int cpid = wait(NULL);
	if(cpid == -1) return;
	
	int iIndex = DynArray_search(childPIDs, &cpid, ChildPID_compare);
	ChildPID_delete(childPIDs, iIndex);

	printf("child %d terminated normally\n", cpid);
}

/* Parent ignore SIGINT signal but children response to it by their behaviour */
void SIGINT_handler(int iSig)
{
	/* Send SIGINT to children */
	int childPID_length = DynArray_getLength(childPIDs);
	int i;
	for(i=0;i<childPID_length;i++){
		int *cpid = (int *)DynArray_get(childPIDs,i);
		kill( *cpid, SIGINT );
	}
}

/* After the first SIGQUIT signal, the next SIGQUIT signal will be handled by SIGQUIT_hanlder2 which is to terminate */
void SIGQUIT_handler2(int iSig)
{
	exit(0);
}


void SIGQUIT_handler1(int iSig)
{
	
	/* Print "Type Ctrl-\ again within 5 seconds to exit" */
	fprintf(stdout,"Type Ctrl-\\ again within 5 seconds to exit.");
	fflush(NULL);
	
	/* 
		Set SIGQUIT the second time to responsible for exit 
		After 5 seconds, SIGQUIT handler would be changed to the dafault (SIGQUIT_handler1)
	*/
	signal(SIGQUIT, SIGQUIT_handler2);
	alarm(5);
	
	/* Send SIGINT to children */
	int childPID_length = DynArray_getLength(childPIDs);
	int i;
	for(i=0;i<childPID_length;i++){
		int *cpid = (int *)DynArray_get(childPIDs,i);
		kill( *cpid, SIGQUIT );
	}
	
}

/* SIGALRM is received when 5 seconds has passed after the first SIGQUIT signal 
	It will change SIGQUIT handler back to the first one*/
void SIGALRM_handler(int iSig)
{
	/* 5 seconds after the first SIGQUIT, it would set SIGQUIT_handler2 to SIGQUIT_handler1 */
	signal(SIGQUIT, SIGQUIT_handler1);
	alarm(0);
}

int main(void)

/* Read a line from stdin, and write to stdout each number and word
   that it contains.  Repeat until EOF.  Return 0 iff successful. */

{
	
	/*
		Make sure that SIGINT, SIGQUIT, SIGALRM are not blocked
	*/
	sigset_t sSet;
	sigemptyset(&sSet);
	sigaddset(&sSet, SIGINT);
	sigaddset(&sSet, SIGQUIT);
	sigaddset(&sSet, SIGALRM);
	sigprocmask(SIG_UNBLOCK, &sSet, NULL);
	
	/*
		acLine: input line buffer
		tokens: Array of tokens obtained from tokenizing acLine
		childPIDs: Array of child process ID invoked by this parent
		iBuiltIn: 1 if the command is a built-in command
		number_token: the number of tokens in the command
	*/
	char acLine[MAX_LINE_SIZE], command[MAX_LINE_SIZE];
	char *line;
	int status, iSuccessful, iBuiltIn;
	
	errMsg = (char *)malloc(50*sizeof(char));

	/*
		initiate child process storage
	*/
	childPIDs = ChildPID_init(0);
	
	/*
		Setup signal handler for each signal
	*/
	signal(SIGCHLD, SIGCHLD_handler);
	signal(SIGINT, SIGINT_handler);
	signal(SIGQUIT, SIGQUIT_handler1);
	signal(SIGALRM, SIGALRM_handler);
	
	/* 
		Open ".ishrc" in the home directory 
		If .ishrc is not found, the file descriptor is set to stdin
	*/
	char *ishrc_filepath = (char *)malloc(MAX_PATH_SIZE * sizeof(char));
	strcpy(ishrc_filepath, getenv("HOME"));
	strcat(ishrc_filepath, "/.ishrc");
	FILE* fd = fopen(ishrc_filepath,"r");

	if (fd == NULL){
		fprintf(stderr,"%s: .ishrc file is not found so the system automatically redirects to stdin.\n",SYSTEM_NAME);
		fd = stdin;
	}
	free(ishrc_filepath);
	
	LOOP:do{
		
		if(fd == stdin){
			fprintf(stdout,"%% ");
			fflush(NULL);
		}
		line = fgets(acLine, MAX_LINE_SIZE, fd); 
		if(line == NULL) continue;

		if(fd != stdin){
			fprintf(stdout,"%% %s", acLine);
			fflush(NULL);
		}
		
		// Allocate memory for tokens
		tokens = DynArray_new(0);
		if (tokens == NULL)
		{
			fprintf(stderr, "Cannot allocate memory\n");
			exit(EXIT_FAILURE);
		}
		
		/* Tokenize string in acLine into token and save in tokens
			It also checks correctness of the syntax. */
		iSuccessful = lexLine(acLine, tokens, errMsg);
		if (!iSuccessful) {
			DynArray_map(tokens, freeToken, NULL);
			DynArray_free(tokens);
			if(strcmp(errMsg,"") != 0) fprintf(stderr,"%s: %s\n",SYSTEM_NAME,errMsg);
			continue;
		}

		iBuiltIn = 1;
		number_token = DynArray_getLength(tokens);

		strcpy(command, Token_getValue(DynArray_get(tokens, 0)) );
		/*
			There are 5 built-in commands: setenv, unsetenv, cd, exit, fg
			We check if the first token is one of the built-in command.
		*/
	
		/* setenv var [value]: set variable var to value. If value is omitted, set to empyty string. */
		if (strcmp(command, "setenv") == 0)
		{
			// value is not determined
			if (number_token == 2 && strcmp( Token_getValue(DynArray_get(tokens, 1)),"") != 0)
			{
				setenv(Token_getValue(DynArray_get(tokens,1)), "", 1);
			}
			else if (number_token == 3 && strcmp(Token_getValue(DynArray_get(tokens,2)),"|") != 0 \
					&& strcmp(Token_getValue(DynArray_get(tokens,2)),"<") != 0 && strcmp(Token_getValue(DynArray_get(tokens,2)),">") != 0)
			{
				setenv(Token_getValue(DynArray_get(tokens,1)), Token_getValue(DynArray_get(tokens,2)), 1);
			}
			else
			{
				fprintf(stderr,"%s: setenv takes one or two parameters\n",SYSTEM_NAME);
			}
		} 
		// unsetenv var: destroy the variable var.
		else if (strcmp(command, "unsetenv") == 0)
		{
			if (number_token == 2 && strcmp(Token_getValue(DynArray_get(tokens,1)),"") != 0 \
				&& strcmp(Token_getValue(DynArray_get(tokens,1)),"|") != 0 && strcmp(Token_getValue(DynArray_get(tokens,1)),"<") != 0 \
				&& strcmp(Token_getValue(DynArray_get(tokens,1)),">") != 0)
			{
				unsetenv(Token_getValue(DynArray_get(tokens,1)));
			}
			else
			{
				fprintf(stderr,"%s: unsetenv takes one parameter\n", SYSTEM_NAME);
			}
		}
		// cd [dir]: change current working directory to dir. If dir is omitted, change to user's HOME directory
		else if (strcmp(command, "cd") == 0)
		{
			if(number_token > 2) fprintf(stderr,"%s: cd: too many arguments\n", SYSTEM_NAME);
			else if(number_token == 2){
				if(chdir(Token_getValue(DynArray_get(tokens,1))) != 0) fprintf(stderr, "%s: %s\n", SYSTEM_NAME, strerror(errno));
			}
			else chdir(getenv("HOME"));
		}
		// exit: exit shell with status 0
		else if (strcmp(command, "exit") == 0)
		{
			DynArray_map(tokens, freeToken, NULL);
			DynArray_free(tokens);
			exit(0);
		}
		/* fg: brings a command that has been running in the background to the foreground. 
		 When there are multiple programs running in the background, it will bring the most recently launched program to the foreground. */
		else if (strcmp(command, "fg") == 0)
		{
			if(ChildPID_getLength(childPIDs) == 0){
				fprintf(stdout,"%s: There is no background process.\n", SYSTEM_NAME);
				goto CLEANUP;
			}
			int lastChild = ChildPID_get(childPIDs, ChildPID_getLength(childPIDs) - 1);
			fprintf(stdout,"[%d] Lastest background process is executing\n", lastChild);
			int pid = waitpid(lastChild,&status,0);
			if(pid == -1) perror("waitpid");
			else {
				fprintf(stdout,"[%d] Done\n", lastChild);
				ChildPID_delete(childPIDs, pid);
			}
		}
		else iBuiltIn = 0;
		
		if(iBuiltIn == 0){
			
			// Clear all I/O buffers
			fflush(NULL);
			
			// Check if it is foreground or background
			int foreground;
			tokens = Token_isBG(tokens,&foreground);
			
			// Fork child process to do the command
			int pid = 1, i, j;
			int *p;
			totalComm = Token_getNumCommand(tokens);

			// TotalComm > 1 means There is at least one pipe
			if(totalComm > 1)
			{
				p = (int *)malloc(2*(totalComm-1)*sizeof(int *));
				for(i=0;i<totalComm-1;i++){
					if(pipe(p + i*2) == -1)
					{
						perror("pipe");
						exit(EXIT_FAILURE);
					}
				}
			}

			/* Iterate through each command in a line
				Create a process for each command from parent process. So, they all are at the same level.
				Then, create pipes for number of total commands - 1 and link them via pipes.*/
			for(i=0;i<totalComm;i++)
			{
				pid = fork();
				if(pid > 0) ChildPID_add(childPIDs, pid);
				else if(pid == 0)
				{
					// int file_descriptor;
					// char *filename;
					// /* Redirect a file descriptor as stdin, if any, for the first process*/
					// if(i==0)
					// {
					// 	/* Open file from redirection, if any */
					// 	filename = (char *)malloc(50 * sizeof(filename));
					// 	tokens = Token_getInput(tokens,filename,&status);
					// 	if(status == 0)
					// 	{
					// 		file_descriptor = open(filename, O_RDONLY);
					// 		if(file_descriptor < 0){
					// 			perror("open read");
					// 			exit(EXIT_FAILURE);
					// 		}

					// 		close(0);
					// 		dup(file_descriptor);
					// 		close(file_descriptor);
					// 	}
					// 	free(filename);
					// }

					// /* Redirect stdout if any */
					// if(i == totalComm-1)
					// {
					// 	filename = (char *)malloc(50 * sizeof(filename));
					// 	tokens = Token_getOutput(tokens,filename,&status);
					// 	if(status == 0)
					// 	{
					// 		file_descriptor = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
					// 		if(file_descriptor < 0){
					// 			perror("open write");
					// 			exit(EXIT_FAILURE);
					// 		}

					// 		close(1);
					// 		dup(file_descriptor);
					// 		close(file_descriptor);
					// 	}
					// 	free(filename);
					// }

					/* Make child read from pipe if it's not the first command */
					if(i!=0)
					{
						if(dup2(p[2*(i-1)],0) < 0){
							perror("dup2");
							exit(EXIT_FAILURE);
						}
					}

					/* Make child write to pipe if it's not the last command */
					if(i!=totalComm-1)
					{
						if(dup2(p[2*i+1],1) < 0){
							perror("dup2");
							exit(EXIT_FAILURE);
						}
					}
					
					if(totalComm>1)
					{
						for(j=0;j<totalComm-1;j++){
							close(p[2*j]);
							close(p[2*j+1]);
						}
						free(p);
					}

					argv = Token_getComm(tokens,i,&number_argv);
					execvp(argv[0],argv);
					fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
					exit(EXIT_FAILURE);
				}
				else 
				{
					perror("fork");
					exit(EXIT_FAILURE);
				}
			}
			
			if( foreground == 1 )
			{
				if(totalComm>1)
				{
					for(j=0;j<totalComm-1;j++){
						close(p[2*j]);
						close(p[2*j+1]);
					}
					free(p);
				}
				for(i=0;i<totalComm;i++)
				{
					pid = wait(&status);
					if(pid == -1) perror("wait");
					else ChildPID_delete(childPIDs, pid);
				}
			}
			// So there is no action for background

			DynArray_map(tokens, freeToken, NULL);
			DynArray_free(tokens);
			
			continue;
		}
		CLEANUP:
			DynArray_map(tokens, freeToken, NULL);
			DynArray_free(tokens);		
	} while(line != NULL);
	if(fd != stdin)
	{
		fclose(fd);
		fd = stdin;
		goto LOOP;
	}
	
	free(errMsg);
	DynArray_map(childPIDs, ChildPID_free, NULL);
	DynArray_free(childPIDs);

	return 0;
}
