/*--------------------------------------------------------------------*/
/* dfa.c                                                              */
/* Original Author: Bob Dondero                                       */
/* Illustrate lexical analysis using a deterministic finite state     */
/* automaton (DFA)                                                    */
/*--------------------------------------------------------------------*/

#include "dynarray.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*--------------------------------------------------------------------*/

enum {MAX_LINE_SIZE = 1024};

enum {FALSE, TRUE};

enum TokenType {TOKEN_WORD, TOKEN_PIPE, TOKEN_BG};

/*--------------------------------------------------------------------*/

/* A Token is either a number or a word, expressed as a string. */

struct Token
{
   enum TokenType eType;
   /* The type of the token. */

   char *pcValue;
   /* The string which is the token's value. */
};

/*--------------------------------------------------------------------*/

void freeToken(void *pvItem, void *pvExtra)

/* Free token pvItem.  pvExtra is unused. */

{
   struct Token *psToken = (struct Token*)pvItem;
   free(psToken->pcValue);
   free(psToken);
}

/*--------------------------------------------------------------------*/

void printWordToken(void *pvItem, void *pvExtra)

/* Print token pvItem to stdout iff it is a word.  pvExtra is
   unused. */

{
   struct Token *psToken = (struct Token*)pvItem;
   if (psToken->eType == TOKEN_WORD)
      printf("%s ", psToken->pcValue);
}

/*--------------------------------------------------------------------*/

struct Token *makeToken(enum TokenType eTokenType,
   char *pcValue)

/* Create and return a Token whose type is eTokenType and whose
   value consists of string pcValue.  Return NULL if insufficient
   memory is available.  The caller owns the Token. */

{
   struct Token *psToken;

   psToken = (struct Token*)malloc(sizeof(struct Token));
   if (psToken == NULL)
      return NULL;

   psToken->eType = eTokenType;

   psToken->pcValue = (char*)malloc(strlen(pcValue) + 1);
   if (psToken->pcValue == NULL)
   {
      free(psToken);
      return NULL;
   }

   strcpy(psToken->pcValue, pcValue);

   return psToken;
}

/*--------------------------------------------------------------------*/

int lexLine(const char *pcLine, DynArray_T oTokens)

/* Lexically analyze string pcLine.  Populate oTokens with the
   tokens that pcLine contains.  Return 1 (TRUE) if successful, or
   0 (FALSE) otherwise.  In the latter case, oTokens may contain
   tokens that were discovered before the error. The caller owns the
   tokens placed in oTokens. */

/* lexLine() uses a DFA approach.  It "reads" its characters from
   pcLine. */

{
   enum LexState {STATE_START, STATE_IN_WORD, STATE_IN_STRING};

   enum LexState eState = STATE_START;

   int iLineIndex = 0;
   int iValueIndex = 0;
   char c;
   char acValue[MAX_LINE_SIZE];
   struct Token *psToken;

   assert(pcLine != NULL);
   assert(oTokens != NULL);

   for (;;)
   {
      /* "Read" the next character from pcLine. */
      c = pcLine[iLineIndex++];

		switch (eState)
		{
			case STATE_START:
				if ((c == '\n') || (c == '\0'))
				{
					if (iValueIndex != 0)
					{
						acValue[iValueIndex] = '\0';
						psToken = makeToken(TOKEN_WORD, acValue);
						if (psToken == NULL)
						{
							fprintf(stderr, "Cannot allocate memory\n");
							return FALSE;
						}
						if (! DynArray_add(oTokens, psToken))
						{
							fprintf(stderr, "Cannot allocate memory\n");
							return FALSE;
						}
						iValueIndex = 0;
						
						return TRUE;
					}
					return TRUE;
				}
				else if (c == '"')
			    {
			       eState = STATE_IN_STRING;
			    }
			    else if (isalpha(c) || isdigit(c))
			    {
			       acValue[iValueIndex++] = c;
			       eState = STATE_IN_WORD;
			    }
			    else if ((c == ' ') || (c == '\t'))
			       eState = STATE_START;
			    else
			    {
			       fprintf(stderr, "Invalid line\n");
			       return FALSE;
			    }
			    break;
			
			case STATE_IN_STRING:
				if(c != '"')
				{
					acValue[iValueIndex++] = c;
					eState = STATE_IN_STRING;
				}
				else
				{
					eState = STATE_START;
				}
				break;
			
			case STATE_IN_WORD:
				if ((c == '\n') || (c == '\0'))
				{
					/* Create a WORD token. */
					acValue[iValueIndex] = '\0';
					psToken = makeToken(TOKEN_WORD, acValue);
					if (psToken == NULL)
					{
						fprintf(stderr, "Cannot allocate memory\n");
						return FALSE;
					}
					if (! DynArray_add(oTokens, psToken))
					{
						fprintf(stderr, "Cannot allocate memory\n");
						return FALSE;
					}
					iValueIndex = 0;
					
					return TRUE;
				}
				else if (isalpha(c) || isdigit(c))
				{
					acValue[iValueIndex++] = c;
					eState = STATE_IN_WORD;
				}
				else if (c == '"')
				{
					eState = STATE_IN_STRING;
				}
				else if ((c == ' ') || (c == '\t'))
				{
					/* Create a WORD token. */
					acValue[iValueIndex] = '\0';
					psToken = makeToken(TOKEN_WORD, acValue);
					if (psToken == NULL)
					{
						fprintf(stderr, "Cannot allocate memory\n");
						return FALSE;
					}
					if (! DynArray_add(oTokens, psToken))
					{
						fprintf(stderr, "Cannot allocate memory\n");
						return FALSE;
					}
					iValueIndex = 0;
					
					eState = STATE_START;
				}
				else
				{
				   fprintf(stderr, "Invalid line\n");
				   return FALSE;
				}
				break;
				
			default:
				assert(FALSE);
		}
	}
}

