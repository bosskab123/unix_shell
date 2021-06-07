#ifndef TOKEN_INCLUDED
#define TOKEN_INCLUDED

struct Token;
typedef enum TokenType {TOKEN_NUMBER, TOKEN_WORD};

/* Free token pvItem.  pvExtra is unused. */
void freeToken(void *pvItem, void *pvExtra);

/* Print token pvItem to stdout iff it is a number.  pvExtra is
   unused. */
void printNumberToken(void *pvItem, void *pvExtra);

/* Print token pvItem to stdout iff it is a word.  pvExtra is
   unused. */
void printWordToken(void *pvItem, void *pvExtra);

/* Create and return a Token whose type is eTokenType and whose
   value consists of string pcValue.  Return NULL if insufficient
   memory is available.  The caller owns the Token. */
struct Token *makeToken(enum TokenType eTokenType,
   char *pcValue)

#endif
