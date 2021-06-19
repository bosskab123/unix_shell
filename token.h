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

char *Token_getInput(DynArray_T oTokens);

char *Token_getOutput(DynArray_T oTokens);

int Token_getNumCommand(DynArray_T oTokens);

char **Token_getComm(DynArray_T oTokens, int index, int *size);

#endif
