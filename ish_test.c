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


int main(void)

/* Read a line from stdin, and write to stdout each number and word
   that it contains.  Repeat until EOF.  Return 0 iff successful. */

{
	/*
		acLine: input line buffer
		tokens: Array of tokens obtained from tokenizing acLine
		vars: Array of variables declared via setenv var [value] where the value of var is value or empty string if value is omitted
		cwd: current working directory
	*/
	char acLine[MAX_LINE_SIZE];
	char command[MAX_LINE_SIZE];
	DynArray_T tokens;
	int iSuccessful, iBuiltIn, number_token;
	
	/* 
		Open ".ishrc" in the home directory 
		If .ishrc is not found, the file descriptor is set to stdin
	*/
	char *ishrc_filepath = (char *)malloc(MAX_PATH_SIZE * sizeof(char));
	strcpy(ishrc_filepath, getenv("HOME"));
	strcat(ishrc_filepath, "/.ishrc");
	FILE* fd = fopen(ishrc_filepath,"r");
	free(ishrc_filepath);
	
	if(fd == NULL) fd=stdin;
	
	/*
		Read each line from the input stream and stored the tokenized string in tokens
	*/
	while (fgets(acLine, MAX_LINE_SIZE, fd) != NULL)
	{
		iBuiltIn = 1;
		// Print out the line 
		printf("%% %s\n", acLine);
		
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
			else if(number_token == 2){
				printf("Move to %s\n", (char *)DynArray_get(tokens,1));
				chdir((char *)DynArray_get(tokens,1));
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
			printf("Bring background to foreground pls\n");
		}
		else iBuiltIn = 0;
		
		if(iBuiltIn == 0){
			
			// Clear all I/O buffers
			fflush(NULL);
			
			// Fork child process to do the command
			pid_t pid;
			pid = fork();
			int status;
			
			if(pid != 0){
				printf("parent\n");
			}
			else{
				
				// Create a char array of token instead of using Dynamic array
				int num_argv = DynArray_getLength(tokens);
				char **argv;
				argv = (char **)malloc(num_argv*sizeof(char *));
				int i;
				for(i=0;i<num_argv;i++){
					argv[i] = (char *)malloc(20*sizeof(char));
					strcpy(argv[i],getTokenValue(DynArray_get(tokens,i)));
				}
				DynArray_map(tokens, freeToken, NULL);
				DynArray_free(tokens);
				
				// Create a process to handle with the program.				
				execvp((char *)argv[0],(char **)argv);
				exit(0);
				
			}
			
			pid = wait(&status);
			printf("child %d has returned\n", pid);
			
			DynArray_map(tokens, freeToken, NULL);
			DynArray_free(tokens);
			
			continue;
		}
		
		DynArray_map(tokens, freeToken, NULL);
		DynArray_free(tokens);
	}
	fclose(fd);

	return 0;
}
