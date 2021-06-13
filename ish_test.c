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

/*--------------------------------------------------------------------*/

#define MAX_LINE_SIZE 1024
#define MAX_PATH_SIZE 1024

DynArray_T childPIDs;
char **argv;
int iSuccessful, iBuiltIn, number_token, number_argv;

void SIGCHLD_handler(int iSig)
{
	
	int cpid = wait(NULL);
	int iIndex = DynArray_search(childPIDs, &cpid, ChildPID_compare);
	int *cp = DynArray_removeAt(childPIDs, iIndex);
	free(cp);
	
}

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

void SIGQUIT_handler2(int iSig)
{
	exit(0);
}


void SIGQUIT_handler1(int iSig)
{
	
	/* Print "Type Ctrl-\ again within 5 seconds to exit" */
	fprintf(stdout,"Type Ctrl-\\ again within 5 seconds to exit.");
	
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
	char acLine[MAX_LINE_SIZE];
	char command[MAX_LINE_SIZE];
	DynArray_T tokens;
	
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
		fprintf(stderr,".ishrc file is not found so the system automatically redirects to stdin.\n");
		fd = stdin;
	}
	free(ishrc_filepath);
	
	/*
		Read each line from the input stream and stored the tokenized string in tokens
	*/
	while (fgets(acLine, MAX_LINE_SIZE, fd) != NULL)
	{
		iBuiltIn = 1;
		// Print out the line 
		printf("%% %s", acLine);
		
		// Allocate memory for tokens
		tokens = DynArray_new(0);
		if (tokens == NULL)
		{
			fprintf(stderr, "Cannot allocate memory\n");
			exit(EXIT_FAILURE);
		}

		// Tokenize string in acLine into token and save in tokens
		iSuccessful = lexLine(acLine, tokens);
		if (!iSuccessful) printf("Something wrong!!\n");

		number_token = DynArray_getLength(tokens);
		
		printf("-----------------------------------\n");
		int a;
		for(a=0;a<number_token;a++){
			printf("token: (%s)\n",getTokenValue(DynArray_get(tokens,a)));
		}
		printf("-----------------------------------\n");
		
		strcpy(command, getTokenValue(DynArray_get(tokens, 0)) );
		/*
			There are 5 built-in commands: setenv, unsetenv, cd, exit, fg
			We check if the first token is one of the built-in command.
		*/
	
		/* setenv var [value]: set variable var to value. If value is omitted, set to empyty string. */
		if (strcmp(command, "setenv") == 0)
		{
			// value is not determined
			if (number_token == 2 && strcmp( getTokenValue(DynArray_get(tokens, 1)),"") != 0)
			{
				setenv(DynArray_get(tokens,1), "", 1);
			}
			else if (number_token == 3 && strcmp(getTokenValue(DynArray_get(tokens,1)),"") != 0 && strcmp(getTokenValue(DynArray_get(tokens,2)),"|") != 0 \
					&& strcmp(getTokenValue(DynArray_get(tokens,2)),"<") != 0 && strcmp(getTokenValue(DynArray_get(tokens,2)),">") != 0)
			{
				setenv(DynArray_get(tokens,1), DynArray_get(tokens,2), 1);
			}
			else if ((number_token == 3 && (strcmp(getTokenValue(DynArray_get(tokens,2)),"|") == 0 || strcmp(getTokenValue(DynArray_get(tokens,2)),"<") == 0 \
					|| strcmp(getTokenValue(DynArray_get(tokens,2)),">") == 0)) \
					|| (number_token > 3 && (strcmp(getTokenValue(DynArray_get(tokens,3)),"|") == 0 || strcmp(getTokenValue(DynArray_get(tokens,3)),"<") == 0 \
					|| strcmp(getTokenValue(DynArray_get(tokens,3)),">") == 0)))
			{
				fprintf(stderr,"Error: Cannot use piped command or file redirection with setenv\n");
			}
			else
			{
				fprintf(stderr,"Correct syntax is \"setenv $var [value]\"\n");
			}
		} 
		// unsetenv var: destroy the variable var.
		else if (strcmp(command, "unsetenv") == 0)
		{
			if (number_token == 2 && strcmp(getTokenValue(DynArray_get(tokens,1)),"") != 0 \
				&& strcmp(getTokenValue(DynArray_get(tokens,1)),"|") != 0 && strcmp(getTokenValue(DynArray_get(tokens,1)),"<") != 0 \
				&& strcmp(getTokenValue(DynArray_get(tokens,1)),">") != 0)
			{
				unsetenv(DynArray_get(tokens,1));
			}
			else if ((number_token == 2 && (strcmp(getTokenValue(DynArray_get(tokens,1)),"|") == 0 || strcmp(getTokenValue(DynArray_get(tokens,1)),"<") == 0 \
					|| strcmp(getTokenValue(DynArray_get(tokens,1)),">") == 0)) \
					|| (number_token > 2 && (strcmp(getTokenValue(DynArray_get(tokens,2)),"|") == 0 || strcmp(getTokenValue(DynArray_get(tokens,2)),"<") == 0 \
					|| strcmp(getTokenValue(DynArray_get(tokens,2)),">") == 0)))
			{
				fprintf(stderr,"Error: Cannot use piped command or file redirection with unsetenv\n");
			}
			else
			{
				fprintf(stderr,"Correct syntax is \"unsetenv $var\"\n");
			}
		}
		// cd [dir]: change current working directory to dir. If dir is omitted, change to user's HOME directory
		else if (strcmp(command, "cd") == 0)
		{
			if(number_token > 2) fprintf(stderr,"-bash: cd: too many arguments\n");
			else if(number_token == 2) chdir(getTokenValue(DynArray_get(tokens,1)));
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
			printf("Bring background to foreground pls\n");
		}
		else iBuiltIn = 0;
		
		if(iBuiltIn == 0){
			
			// Clear all I/O buffers
			fflush(NULL);
			
			// Check if it is foreground or background
			int foreground = 1;
			if( strcmp("&",getTokenValue(DynArray_get(tokens,number_token-1))) == 0 ) foreground = 0;
			
			// Fork child process to do the command
			pid_t pid;
			pid = fork();
			int status;
			
			if(pid != 0)
			{
				ChildPID_add(childPIDs, pid);
			}
			else
			{	
				// Create a char array of token instead of using Dynamic array
				number_argv = DynArray_getLength(tokens);
				argv = (char **)malloc(number_argv*sizeof(char *));
				int i;
				for(i=0;i<number_argv;i++){
					argv[i] = (char *)malloc(20*sizeof(char));
					strcpy(argv[i],getTokenValue(DynArray_get(tokens,i)));
				}
				DynArray_map(tokens, freeToken, NULL);
				DynArray_free(tokens);
				
				// Create a process to handle with the program.
				execvp(argv[0],argv);
				
				// If there is an error, print an error message and terminate the program.
				for(i=0;i<number_argv;i++){
					free(argv[i]);
				}
				free(argv);
				fprintf(stderr,"Error for running %s\n", command);
				exit(0);
				
			}
			
			if( foreground == 1 ){
				pid = wait(&status);
				//printf("child %d has returned\n", pid);
			}
			
			DynArray_map(tokens, freeToken, NULL);
			DynArray_free(tokens);
			
			continue;
		}
		
		DynArray_map(tokens, freeToken, NULL);
		DynArray_free(tokens);
	}
	fclose(fd);
	
	DynArray_map(tokens, ChildPID_free, NULL);
	DynArray_free(childPIDs);

	return 0;
}
