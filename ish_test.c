/*--------------------------------------------------------------------*/
/* dfa.c                                                              */
/* Original Author: Bob Dondero                                       */
/* Illustrate lexical analysis using a deterministic finite state     */
/* automaton (DFA)                                                    */
/*--------------------------------------------------------------------*/

#include "dynarray.h"
#include "token.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
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
	*/
	char acLine[MAX_LINE_SIZE];
	DynArray_T tokens;
	
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
		

		DynArray_map(tokens, freeToken, NULL);
		DynArray_free(tokens);
	}
	fclose(fd);

	return 0;
}
