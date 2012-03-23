/*--------------------------------------------------------------------*/
/* lex.h                                                              */
/* Author: Shreshth Singhal (netID: ssinghal)                         */
/* Lexical analyzer for an interactive shell                          */
/*--------------------------------------------------------------------*/

#ifndef LEX_INCLUDED
#define LEX_INCLUDED

#include "dynarray.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef MAX_LINE_SIZE
#define MAX_LINE_SIZE 1024
#endif

/*--------------------------------------------------------------------*/

typedef struct Token *Token_T;
/* A token is a string which is either a special character (like 
   < or >), or a normal word (like a command or an argument. */

enum TokenType {TOKEN_QUOTE, TOKEN_WORD};
typedef enum TokenType TokenType;

/*--------------------------------------------------------------------*/

TokenType Lex_returnType(void *pvItem);
/* Returns type of token pvItem. */

char *Lex_returnValue(void *pvItem);
/* Returns value of token pvItem. */

void Lex_freeToken(void *pvItem, void *pvExtra);   
/* Free token pvItem. pvExtra is unused. */

int Lex_lexLine(const char *pcLine, DynArray_T oTokens, char *pcProgName);   
/* Lexically analyze string pcLine. Populate oTokens with the
   tokens that pcLine contains. Return 1 (TRUE) if successful, or
   0 (FALSE) otherwise. In the latter case, oTokens may contain
   tokens that were discovered before the error. The caller owns
   the tokens placed in oTokens. Note that it returns TRUE if a line
   is entirely a comment but does not add to oTokens. Also note that
   function requires calling program's name for error-checking. */

#endif






