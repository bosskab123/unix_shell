#ifndef TOKEN_INCLUDED
#define TOKEN_INCLUDED

enum TokenType {TOKEN_WORD, TOKEN_P, TOKEN_BG, TOKEN_RL, TOKEN_RR};

/* Free token pvItem.  pvExtra is unused. */
void freeToken(void *pvItem, void *pvExtra);

/* Print token pvItem to stdout iff it is a number.  pvExtra is
   unused. */
void printNumberToken(void *pvItem, void *pvExtra);

/* Print token pvItem to stdout iff it is a word.  pvExtra is
   unused. */
void printWordToken(void *pvItem, void *pvExtra);

/* Return type of the token to caller */
enum TokenType getTokenType(void *pvItem);

/* Return value of the token to caller */
char * getTokenValue(void *pvItem);

/* Create and return a Token whose type is eTokenType and whose
   value consists of string pcValue.  Return NULL if insufficient
   memory is available.  The caller owns the Token. */
struct Token *makeToken(enum TokenType eTokenType,
   char *pcValue);

/* Lexically analyze string pcLine.  Populate oTokens with the
   tokens that pcLine contains.  Return 1 (TRUE) if successful, or
   0 (FALSE) otherwise.  In the latter case, oTokens may contain
   tokens that were discovered before the error. The caller owns the
   tokens placed in oTokens. */

/* lexLine() uses a DFA approach.  It "reads" its characters from
   pcLine. */
int lexLine(const char *pcLine, DynArray_T oTokens, char *errMsg);

/* Check if this set of tokens is a background process (end with &) 
   And eliminate the '&' out grom the array
*/
DynArray_T Token_isBG(DynArray_T oTokens, int *status);

/* Get input file descriptor */
DynArray_T Token_getInput(DynArray_T oTokens, char *filename, int *status);

/* Get output file descriptor */
DynArray_T Token_getOutput(DynArray_T oTokens, char *filename, int * status);

/* Get the total number of command in a set of tokens*/
int Token_getNumCommand(DynArray_T oTokens);

/* Get ith command in the set of tokens*/
char **Token_getComm(DynArray_T oTokens, int index, int *size);

#endif
