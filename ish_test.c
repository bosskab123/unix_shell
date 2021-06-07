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
	
	char acLine[MAX_LINE_SIZE];
	DynArray_T oTokens;
	int iSuccessful;
	/* Open ".ishrc" in the home directory 
	If .ishrc is not found, the file descriptor is set to stdin*/
	char *usr_home = (char *)malloc(MAX_PATH_SIZE * sizeof(char));
	strcpy(usr_home,getenv("HOME"));
	char *ishrc_filepath = (char *)malloc(MAX_PATH_SIZE * sizeof(char));
	strcpy(ishrc_filepath, usr_home);
	strcat(ishrc_filepath, "/.ishrc");
	printf("ishrc filepath: %s\n",ishrc_filepath);
	int fd = open(".ishrc",O_RDONLY);
	if(fd == -1) fd = 0;
	
	printf("------------------------------------\n");
	while (fgets(acLine, MAX_LINE_SIZE, stdin) != NULL)
	{
		oTokens = DynArray_new(0);
		if (oTokens == NULL)
		{
			fprintf(stderr, "Cannot allocate memory\n");
			exit(EXIT_FAILURE);
		}

		iSuccessful = lexLine(acLine, oTokens);
		if (iSuccessful)
		{
			printf("Numbers:  ");
			DynArray_map(oTokens, printNumberToken, NULL);
			printf("\n");

			printf("Words:  ");
        	DynArray_map(oTokens, printWordToken, NULL);
        	printf("\n");
		}
    	printf("------------------------------------\n");

		DynArray_map(oTokens, freeToken, NULL);
		DynArray_free(oTokens);
	}

	return 0;
}
