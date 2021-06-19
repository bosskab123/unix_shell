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

void SIGCHLD_handler(int iSig)
{
	int cpid = wait(NULL);
	if(cpid == -1) return;
	
	int iIndex = DynArray_search(childPIDs, &cpid, ChildPID_compare);
	ChildPID_delete(childPIDs, iIndex);

	printf("child %d terminated normally\n", cpid);
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
	char *line;
	char command[MAX_LINE_SIZE];
	int status,iSuccessful, iBuiltIn;
	
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

	errMsg = (char *)malloc(50*sizeof(char));

	if (fd == NULL){
		fprintf(stderr,"%s: .ishrc file is not found so the system automatically redirects to stdin.\n",SYSTEM_NAME);
		fd = stdin;
	}
	free(ishrc_filepath);
	
	LOOP:do{
		
		if(fd == stdin) fprintf(stdout,"%% ");
		line = fgets(acLine, MAX_LINE_SIZE, fd); 
		if(line == NULL) continue;

		if(fd != stdin) fprintf(stdout,"%% %s", acLine);
		fflush(NULL);
		
		// Allocate memory for tokens
		tokens = DynArray_new(0);
		if (tokens == NULL)
		{
			fprintf(stderr, "Cannot allocate memory\n");
			exit(EXIT_FAILURE);
		}
		
		// Tokenize string in acLine into token and save in tokens
		
		iSuccessful = lexLine(acLine, tokens, errMsg);
		if (!iSuccessful) {
			DynArray_map(tokens, freeToken, NULL);
			DynArray_free(tokens);
			if(strcmp(errMsg,"") != 0) fprintf(stderr,"%s: %s\n",SYSTEM_NAME,errMsg);
			continue;
		}

		iBuiltIn = 1;
		number_token = DynArray_getLength(tokens);
		
		/*
		// Special purpose: To check the generated tokens
		int it;
		for(it=0;it<number_token;it++){
			printf("Token %d: (%s)\n",it,getTokenValue(DynArray_get(tokens,it)));
		}
		*/

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
				setenv(getTokenValue(DynArray_get(tokens,1)), "", 1);
			}
			else if (number_token == 3 && strcmp(getTokenValue(DynArray_get(tokens,2)),"|") != 0 \
					&& strcmp(getTokenValue(DynArray_get(tokens,2)),"<") != 0 && strcmp(getTokenValue(DynArray_get(tokens,2)),">") != 0)
			{
				setenv(getTokenValue(DynArray_get(tokens,1)), getTokenValue(DynArray_get(tokens,2)), 1);
			}
			else
			{
				fprintf(stderr,"%s: setenv takes one or two parameters\n",SYSTEM_NAME);
			}
		} 
		// unsetenv var: destroy the variable var.
		else if (strcmp(command, "unsetenv") == 0)
		{
			if (number_token == 2 && strcmp(getTokenValue(DynArray_get(tokens,1)),"") != 0 \
				&& strcmp(getTokenValue(DynArray_get(tokens,1)),"|") != 0 && strcmp(getTokenValue(DynArray_get(tokens,1)),"<") != 0 \
				&& strcmp(getTokenValue(DynArray_get(tokens,1)),">") != 0)
			{
				unsetenv(getTokenValue(DynArray_get(tokens,1)));
			}
			else
			{
				fprintf(stderr,"%s: correct syntax is \"unsetenv $var\"\n", SYSTEM_NAME);
			}
		}
		// cd [dir]: change current working directory to dir. If dir is omitted, change to user's HOME directory
		else if (strcmp(command, "cd") == 0)
		{
			if(number_token > 2) fprintf(stderr,"%s: cd: too many arguments\n", SYSTEM_NAME);
			else if(number_token == 2){
				if(chdir(getTokenValue(DynArray_get(tokens,1))) != 0) fprintf(stderr, "%s: %s\n", SYSTEM_NAME, strerror(errno));
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
			int foreground = 1;
			if( strcmp("&",getTokenValue(DynArray_get(tokens,number_token-1))) == 0 ){
				foreground = 0;
				DynArray_removeAt(tokens,number_token-1);
			}
			
			// Fork child process to do the command
			int pid = 1, p[2], i, j;
			totalComm = Token_getNumCommand(tokens);

			/* Check each command set and total number of command */
			// printf("================\n");
			// printf("totalComm: %d\n", totalComm);
			// for(i=0;i<totalComm;i++){
			// 	int num_argv;
			// 	argv = Token_getComm(tokens,i,&num_argv);
			// 	for(j=0;j<num_argv;j++){
			// 		printf("%s ",argv[j]);
			// 	}
			// 	printf("\n");
			// }
			// printf("================\n");

			// TotalComm > 1 means There is at least one pipe
			printf("yep\n");
			if(totalComm > 1)
			{
				pipe(p);
				if(pipe(p) == -1)
				{
					perror("pipe");
					exit(EXIT_FAILURE);
				}
			}

			for(i=0;i<totalComm;i++)
			{
				if( pid != 0)
				{
					pid = fork();
					if(pid != 0) ChildPID_add(childPIDs, pid);
					else 
					{
						int file_descriptor;
						char *filename;
						
						/* Redirect stdin if any for the first process*/
						if(i==0)
						{
							filename = Token_getInput(tokens);
							if(filename != NULL)
							{
								file_descriptor = open(filename, O_RDONLY);
								if(file_descriptor < 0){
									perror("open read");
									exit(EXIT_FAILURE);
								}

								close(0);
								dup(file_descriptor);
								close(file_descriptor);
							}
						}

						/* Redirect stdout if any */
						else if(i==totalComm-1)
						{
							filename = Token_getOutput(tokens);
							if(filename != NULL)
							{
								file_descriptor = open(filename, O_WRONLY | O_CREAT, 0600);
								if(file_descriptor < 0){
									perror("open write");
									exit(EXIT_FAILURE);
								}

								close(1);
								dup(file_descriptor);
								close(file_descriptor);
							}
						}

					 	argv = Token_getComm(tokens,i,&number_argv);

						// Create a char array of token instead of using Dynamic array
						execvp(argv[0],argv);
						fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
						for(j=0;j<number_argv+1;j++){
							free(argv[j]);
						}
						free(argv);
						exit(EXIT_FAILURE);
					}
				}
			}
			
			if( foreground == 1 ){
				pid = wait(&status);
				if(pid == -1) perror("wait");
				else ChildPID_delete(childPIDs, pid);
			}
			// So there is no action for background

			DynArray_map(tokens, freeToken, NULL);
			DynArray_free(tokens);
			
			continue;
		}
		
		DynArray_map(tokens, freeToken, NULL);
		DynArray_free(tokens);		
	} while(line != NULL);
	if(fd != stdin)
	{
		fclose(fd);
		fd = stdin;
		goto LOOP;
	}
	
	DynArray_map(childPIDs, ChildPID_free, NULL);
	DynArray_free(childPIDs);

	return 0;
}
