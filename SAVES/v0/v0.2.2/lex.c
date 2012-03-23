/*--------------------------------------------------------------------*/
/* lex.c                                                              */
/* Author: Shreshth Singhal (netID: ssinghal)                         */
/* Lexical analyzer for an interactive shell                          */
/*--------------------------------------------------------------------*/

#include "lex.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

/*--------------------------------------------------------------------*/

struct Token

/* A token is a string which is either a special character (like 
   < or >), or a normal word (like a command or an argument. */

{
      TokenType eType;
      /* Type of the token */
      
      char *pcValue;
      /* The string which is the token's value */
};

/*--------------------------------------------------------------------*/

TokenType Lex_returnType(void *pvItem)

/* Returns type of token pvItem. */

{
   Token_T psToken = NULL;

   assert(pvItem != NULL);

   psToken = (Token_T)pvItem;
   
   assert((psToken->eType == TOKEN_QUOTE) ||
          (psToken->eType == TOKEN_WORD));

   return psToken->eType;
}

/*--------------------------------------------------------------------*/

char *Lex_returnValue(void *pvItem)

/* Returns value of token pvItem. */

{
   Token_T psToken = NULL;

   assert(pvItem != NULL);

   psToken = (Token_T)pvItem;
   return psToken->pcValue;
}

/*--------------------------------------------------------------------*/

void Lex_freeToken(void *pvItem, void *pvExtra)
   
/* Free token pvItem. pvExtra is unused. */
   
{
   Token_T psToken = NULL;

   assert(pvItem != NULL);

   psToken = (Token_T)pvItem;
   free(psToken->pcValue);
   free(psToken);
}

/*--------------------------------------------------------------------*/

static Token_T Lex_makeToken(TokenType eTokenType, char *pcValue)
   
/* Create and return a Token whose type is eTokenType and whose
   value consists of string pcValue. Return NULL if insufficient
   memory is available. The caller owns the Token. */

{
   Token_T psToken = NULL;

   assert(pcValue != NULL);

   psToken = (Token_T)malloc(sizeof(struct Token));
   /* Insufficient memory. */
   if (psToken == NULL)
      return NULL;
   
   psToken->eType = eTokenType;
   
   psToken->pcValue = (char*)malloc(strlen(pcValue) + 1);
   /* Insufficient memory. */
   if (psToken->pcValue == NULL)
   {
      free(psToken);
      return NULL;
   }
   
   strcpy(psToken->pcValue, pcValue);
   
   return psToken;
}

/*--------------------------------------------------------------------*/

int Lex_lexLine(const char *pcLine, DynArray_T oTokens, 
                char *pcProgName)
   
/* Lexically analyze string pcLine. Populate oTokens with the
   tokens that pcLine contains. Return 1 (TRUE) if successful, or
   0 (FALSE) otherwise. In the latter case, oTokens may contain
   tokens that were discovered before the error. The caller owns
   the tokens placed in oTokens. Note that it returns TRUE if a line
   is entirely a comment but does not add to oTokens. */
   
/* lexLine() uses DFA approach. It "reads" its characters from
   psLine. */
   
{
   char c;
   Token_T psToken = NULL;

   enum LexState {STATE_START, STATE_IN_WORD, STATE_IN_QUOTE};
   enum LexState eState = STATE_START;
   /* DFA states. */
   
   int iLineIndex = 0;
   /* Index for the line read from stdin. */

   int iValueIndex = 0;
   /* Index for each token created from the line. */

   char acValue[MAX_LINE_SIZE];
   /* To store token's string value. */

   assert(pcLine != NULL);
   assert(oTokens != NULL);
   assert(pcProgName != NULL);
   
   for (;;)
   {
      /* "Read" the next character from pcLine. */
      c = pcLine[iLineIndex++];
      
      switch (eState)
      {
         case STATE_START:
            /* Start state (not in a token). */
            if ((c == '\n') || (c == '\0')) /* End of line. */
               return TRUE;
            else if (c == '"') /* Beginning of quoted token. */
               eState = STATE_IN_QUOTE;
            else if (isspace((int)c)) /* Ignore non-quoted spaces. */
               eState = STATE_START;
            else if ((c == '>') || (c == '<')) 
            {
               /* stdin/stdout redirection. */
               /* Create a WORD token. */
               acValue[0] = c;
               acValue[1] = '\0';
               psToken = Lex_makeToken(TOKEN_WORD, acValue);
               if (psToken == NULL)
               {
                  fprintf(stderr, "%s: cannot allocate memory\n", 
                          pcProgName);
                  return FALSE;
               }
               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", 
                          pcProgName);
                  return FALSE;
               }
               iValueIndex = 0;
               eState = STATE_START;
            }
            else /* Any other character. */
            {
               acValue[iValueIndex++] = c;
               eState = STATE_IN_WORD;
            }
            break;
                        
         case STATE_IN_QUOTE:
            /* Quoted token. */
            if (c == '"') /* Matching ending quote. */
            {
               /* Create a QUOTE token. */
               acValue[iValueIndex] = '\0';
               psToken = Lex_makeToken(TOKEN_QUOTE, acValue);
               if (psToken == NULL)
               {
                  fprintf(stderr, "%s: cannot allocate memory\n", 
                          pcProgName);
                  return FALSE;
               }
               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "%s: cannot allocate memory\n", 
                          pcProgName);
                  return FALSE;
               }
               iValueIndex = 0;
               eState = STATE_START;
            }
            else if ((c == '\n') || (c == '\0')) 
            {
               /* Unterminated quotes. */
               fprintf(stderr, "%s: unmatched quote\n", pcProgName);
               return FALSE;
            }
            else /* Any other character. */
            {
               acValue[iValueIndex++] = c;
               eState = STATE_IN_QUOTE;
            }
            break;
            
         case STATE_IN_WORD:
            /* Non-quoted token. */
            if ((c == '\n') || (c == '\0')) /* End of line. */
            {
               /* Create a WORD token. */
               acValue[iValueIndex] = '\0';
               psToken = Lex_makeToken(TOKEN_WORD, acValue);
               if (psToken == NULL)
               {
                  fprintf(stderr, "%s: cannot allocate memory\n", 
                          pcProgName);
                  return FALSE;
               }
               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "%s: cannot allocate memory\n", 
                          pcProgName);
                  return FALSE;
               }
               iValueIndex = 0;
               return TRUE;
            }
            else if (isspace((int)c)) /* Whitespace. */
            {
               /* Create a WORD token. */
               acValue[iValueIndex] = '\0';
               psToken = Lex_makeToken(TOKEN_WORD, acValue);
               if (psToken == NULL)
               {
                  fprintf(stderr, "%s: cannot allocate memory\n", 
                          pcProgName);
                  return FALSE;
               }
               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "%s: cannot allocate memory\n", 
                          pcProgName);
                  return FALSE;
               }
               iValueIndex = 0;
               eState = STATE_START;
            }
            else if (c == '"') /* Quote within a word. */
            {
               /* If there is a quote in a word, then the whole word
                  switches to a quoted token. */
               eState = STATE_IN_QUOTE;
            }
            else /* Any other character. */
            {
               acValue[iValueIndex++] = c;
               eState = STATE_IN_WORD;
            }
            break;
            
         default:
            assert(FALSE);
      }
   }
}
