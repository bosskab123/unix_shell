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

enum TokenType {TOKEN_WORD, TOKEN_P, TOKEN_BG, TOKEN_RL, TOKEN_RR};

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
	assert(pvItem != NULL);
   struct Token *psToken = (struct Token*)pvItem;
   free(psToken->pcValue);
   free(psToken);
}

/*--------------------------------------------------------------------*/

void printWordToken(void *pvItem, void *pvExtra)

/* Print token pvItem to stdout iff it is a word.  pvExtra is
   unused. */

{
	assert(pvItem != NULL);
   struct Token *psToken = (struct Token*)pvItem;
   if (psToken->eType == TOKEN_WORD)
      printf("%s ", psToken->pcValue);
}

/*--------------------------------------------------------------------*/

enum TokenType getTokenType(void *pvItem)

/* Return value of the token to caller */

{
	assert(pvItem != NULL);
	struct Token *psToken = (struct Token*)pvItem;
	return psToken->eType;
}

/*--------------------------------------------------------------------*/

char *getTokenValue(void *pvItem)

/* Return value of the token to caller */

{
	assert(pvItem != NULL);
	struct Token *psToken = (struct Token*)pvItem;
	return psToken->pcValue;
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

int lexLine(const char *pcLine, DynArray_T oTokens, char *errMsg)

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
   int num_token = 0;
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
							strcpy(errMsg,"Cannot allocate memory");
							return FALSE;
						}
						if (! DynArray_add(oTokens, psToken))
						{
							strcpy(errMsg,"Cannot allocate memory");
							return FALSE;
						}
						iValueIndex = 0;
						
						goto ANALYZE;
					}
					else if(iLineIndex==0){
						strcpy(errMsg,"");
						return FALSE;
					}
					else goto ANALYZE;
				}
				else if (c == '"')
			    {
			       eState = STATE_IN_STRING;
			    }
			    else if (isspace(c))
			       eState = STATE_START;
			    else if (c == '&' || c == '|' || c == '>' || c == '<')
				{
					acValue[iValueIndex] = c;
					switch (c)
					{
						case '&': 
							psToken = makeToken(TOKEN_BG, acValue);
							break;
						case '|': 
							psToken = makeToken(TOKEN_P, acValue);
							break;
						case '<':
							psToken = makeToken(TOKEN_RL, acValue);
							break;
						case '>':
							psToken = makeToken(TOKEN_RR, acValue);
							break;
						default:
							assert(FALSE);
					}
					if (psToken == NULL)
					{
						strcpy(errMsg,"Cannot allocate memory");
						return FALSE;
					}
					if (! DynArray_add(oTokens, psToken))
					{
						strcpy(errMsg,"Cannot allocate memory");
						return FALSE;
					}
					iValueIndex = 0;
					eState = STATE_START;
				}
			    else
			    {
			       acValue[iValueIndex++] = c;
			       eState = STATE_IN_WORD;
			    }
			    break;
			
			case STATE_IN_STRING:
				if(c != '"')
				{
					acValue[iValueIndex++] = c;
					eState = STATE_IN_STRING;
				}
				else if(c == '\n' || c == '\0')
				{
					strcpy(errMsg,"Could not find quote pair");
					return FALSE;
				}
				else
				{
					eState = STATE_IN_WORD;
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
						strcpy(errMsg,"Cannot allocate memory");
						return FALSE;
					}
					if (! DynArray_add(oTokens, psToken))
					{
						strcpy(errMsg,"Cannot allocate memory");
						return FALSE;
					}
					iValueIndex = 0;
					
					goto ANALYZE;
				}
				else if (c == '"')
				{
					eState = STATE_IN_STRING;
				}
				else if (isspace(c))
				{
					/* Create a WORD token. */
					acValue[iValueIndex] = '\0';
					psToken = makeToken(TOKEN_WORD, acValue);
					if (psToken == NULL)
					{
						strcpy(errMsg,"Cannot allocate memory");
						return FALSE;
					}
					if (! DynArray_add(oTokens, psToken))
					{
						strcpy(errMsg,"Cannot allocate memory");
						return FALSE;
					}
					iValueIndex = 0;
					
					eState = STATE_START;
				}
				else if (c == '&' || c == '|' || c == '>' || c == '<')
				{
					acValue[iValueIndex] = c;
					switch (c)
					{
						case '&': 
							psToken = makeToken(TOKEN_BG, acValue);
							break;
						case '|': 
							psToken = makeToken(TOKEN_P, acValue);
							break;
						case '<':
							psToken = makeToken(TOKEN_RL, acValue);
							break;
						case '>':
							psToken = makeToken(TOKEN_RR, acValue);
							break;
						default:
							assert(FALSE);
					}
					if (psToken == NULL)
					{
						strcpy(errMsg,"Cannot allocate memory");
						return FALSE;
					}
					if (! DynArray_add(oTokens, psToken))
					{
						strcpy(errMsg,"Cannot allocate memory");
						return FALSE;
					}
					iValueIndex = 0;
					eState = STATE_START;
				}
				else
				{
					acValue[iValueIndex++] = c;
					eState = STATE_IN_WORD;
				}
				break;
				
			default:
				assert(FALSE);
		}
	}
	
	ANALYZE:
		num_token = DynArray_getLength(oTokens);
		int i;
		for(i=0;i<num_token;i++){
			if(getTokenType(DynArray_get(oTokens,i)) == TOKEN_BG && i!=num_token-1){
				strcpy(errMsg,"Wrong Syntax using &");
				return FALSE;
			}
			else if(getTokenType(DynArray_get(oTokens,i)) == TOKEN_RL ) {
				if(i>0) {
					if(getTokenType(DynArray_get(oTokens,i-1)) != TOKEN_WORD) {
						strcpy(errMsg,"Pipe or redirection destination is not specified");
						return FALSE;
					}
				}
				else {
					strcpy(errMsg,"Pipe or redirection destination is not specified");
					return FALSE;
				}
			}
			else if(getTokenType(DynArray_get(oTokens,i)) == TOKEN_RR ) {
				if(i<num_token-1) {
					if(getTokenType(DynArray_get(oTokens,i+1)) != TOKEN_WORD) {
						strcpy(errMsg,"Pipe or redirection destination is not specified");
						return FALSE;
					}
				}
				else {
					strcpy(errMsg,"Pipe or redirection destination is not specified");
					return FALSE;
				}
			}
			else if(getTokenType(DynArray_get(oTokens,i)) == TOKEN_P ) {
				if(i>0 && i<num_token-1) {
					if(getTokenType(DynArray_get(oTokens,i-1)) != TOKEN_WORD || getTokenType(DynArray_get(oTokens,i+1)) != TOKEN_WORD) {
						strcpy(errMsg,"Pipe or redirection destination is not specified");
						return FALSE;
					}
				}
				else {
					strcpy(errMsg,"Pipe or redirection destination is not specified");
					return FALSE;
				}
			}
		}
		return TRUE;
	
}

