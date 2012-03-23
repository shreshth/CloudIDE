/*--------------------------------------------------------------------*/
/* syn.c                                                              */
/* Author: Shreshth Singhal (netID: ssinghal)                         */
/* Syntactical analyzer for interactive shell                         */
/*--------------------------------------------------------------------*/

#include "syn.h"
#include "lex.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*--------------------------------------------------------------------*/

struct Cmd

/* A command word is a string which is a part of a shell command
   along with its type */

{
      CmdType eType;
      /* Type of command string. */

      char *pcValue;
      /* The string which is the command string's value. */
};

/*--------------------------------------------------------------------*/

CmdType Syn_returnType(void *pvItem)

/* Return the command word's type. */

{
   Cmd_T psCmd = NULL;
   
   assert(pvItem != NULL);

   psCmd = (Cmd_T)pvItem;

   assert((psCmd->eType == CMD_CMD) ||
          (psCmd->eType == CMD_ARG) ||
	  (psCmd->eType == CMD_STDIN) ||
          (psCmd->eType == CMD_STDOUT));

   return psCmd->eType;
}

/*--------------------------------------------------------------------*/

char *Syn_returnValue(void *pvItem)

/* Return the command word's value. */

{
   Cmd_T psCmd = NULL;
   
   assert(pvItem != NULL);

   psCmd = (Cmd_T)pvItem;
   return psCmd->pcValue;
}

/*--------------------------------------------------------------------*/

void Syn_freeCmd(void *pvItem, void *pvExtra)

/* Free command word pvItem. pvExtra is unused. */

{
   Cmd_T psCmd = NULL;
   
   assert(pvItem != NULL);

   psCmd = (Cmd_T)pvItem;
   free(psCmd->pcValue);
   free(psCmd);
}

/*--------------------------------------------------------------------*/

static Cmd_T Syn_makeCmd(CmdType eCmdType, char *pcValue)

/* Create and return a Command word whose type is eCmdType and whose
   value consists of string pcValue. Return NULL if insufficient
   memory is available. The caller owns the command. */

{
   Cmd_T psCmd = NULL;

   psCmd = (Cmd_T)malloc(sizeof(struct Cmd));
   /* Insufficient memory. */
   if (psCmd == NULL)
      return NULL;

   psCmd->eType = eCmdType;

   /* Handle NULL stdin and stdout redirections. */
   if (pcValue == NULL)
      psCmd->pcValue = NULL;
   else
   {
      
      psCmd->pcValue = (char*)malloc(strlen(pcValue) + 1);
      /* Insufficient memory. */
      if (psCmd->pcValue == NULL)
      {
         free(psCmd);
         return NULL;
      }
      strcpy(psCmd->pcValue, pcValue);
   }

   return psCmd;
}

/*--------------------------------------------------------------------*/

int Syn_synLine(DynArray_T oTokens, DynArray_T oCmds, char *pcProgName)

/* Syntactically analyze the token array oTokens. Populate oCmds
   with tokens that oTokens contains along with their type. 
   Return 1 (TRUE) if successful, or 0 (FALSE) otherwise. In the 
   latter case, oCmds may contain tokens that were discovered before 
   the error. The caller owns the tokens placed in oCmds. */ 

{
   int iLength;
   int i = 0;
   Token_T psToken = NULL;
   Cmd_T psCmd = NULL;

   int iStdinFlag = FALSE;
   /* Flag if stdin redirection. */

   int iStdoutFlag = FALSE;
   /* Flag if stdout redirection. */

   assert(oTokens != NULL);
   assert(oCmds != NULL);
   assert(pcProgName != NULL);

   iLength = DynArray_getLength(oTokens);
   if (iLength == 0)
   {
      fprintf(stderr, "%s: empty token array\n", pcProgName);
      return FALSE;
   }

   /* First word of command line. */
   psToken = (Token_T)DynArray_get(oTokens, 0);
   /* Quoted command name added without check. */
   if (Lex_returnType(psToken) == TOKEN_QUOTE)
   {
      psCmd = Syn_makeCmd(CMD_CMD, Lex_returnValue(psToken));
      if (psCmd == NULL)
      {
         fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
         return FALSE;
      }
      if (! DynArray_add(oCmds, psCmd))
      {
         fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
         return FALSE;
      }
   }
   /* Missing command name. */
   else if ((strcmp(Lex_returnValue(psToken), ">") == 0) ||
            (strcmp(Lex_returnValue(psToken), "<") == 0))
   {
      fprintf(stderr, "%s: missing command name\n", pcProgName);
      return FALSE;
   }
   /* Add command name. */
   else
   {
      psCmd = Syn_makeCmd(CMD_CMD, Lex_returnValue(psToken));
      if (psCmd == NULL)
      {
         fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
         return FALSE;
      }
      if (! DynArray_add(oCmds, psCmd))
      {
         fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
         return FALSE;
      }
   }
   
   /* Rest of the command line words. */
   for (i = 1; i < iLength; i++)
   {
      psToken = (Token_T)DynArray_get(oTokens, i);
      /* Quoted arguments added without check. */
      if (Lex_returnType(psToken) == TOKEN_QUOTE)
      {
         psCmd = Syn_makeCmd(CMD_ARG, Lex_returnValue(psToken));
         if (psCmd == NULL)
         {
            fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
            return FALSE;
         }
         if (! DynArray_add(oCmds, psCmd))
         {
            fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
            return FALSE;
         }
      }
      /* stdin redirection. */
      else if (strcmp(Lex_returnValue(psToken), "<") == 0)
      {
         /* Multiple redirection. */
         if (iStdinFlag)
         {
            fprintf(stderr, 
                    "%s: multiple redirection of standard input\n", 
                    pcProgName);
            return FALSE;
         }
         /* Index of redirection target. */
         iStdinFlag = i + 1;
         /* Avoid the redirection target for now. */
         i++;
      }
      /* stdout redirection. */
      else if (strcmp(Lex_returnValue(psToken), ">") == 0)
      {
         /* Multiple redirection. */
         if (iStdoutFlag)
         {
            fprintf(stderr, 
                    "%s: multiple redirection of standard output\n", 
                    pcProgName);
            return FALSE;
         }
         /* Index of redirection target. */
         iStdoutFlag = i + 1;
         /* Avoid the redirection target for now. */
         i++;
      }
      /* Add argument. */
      else
      {
         psCmd = Syn_makeCmd(CMD_ARG, Lex_returnValue(psToken));
         if (psCmd == NULL)
         {
            fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
            return FALSE;
         }
         if (! DynArray_add(oCmds, psCmd))
         {
            fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
            return FALSE;
         }
      }
   }

   /* Check stdin redirection. */
   /* No stdin redirection. */
   if (!iStdinFlag)
   {
      psCmd = Syn_makeCmd(CMD_STDIN, NULL);
      if (psCmd == NULL)
      {
         fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
         return FALSE;
      }
      if (! DynArray_add(oCmds, psCmd))
      {
         fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
         return FALSE;
      }
   }
   /* If there is stdin redirection. */
   else
   {
      /* stdin redirection without file name. */
      if (iStdinFlag >= iLength)
      {
         fprintf(stderr, 
                 "%s: standard input redirection without file name\n",
                 pcProgName);
         return FALSE;
      }
      psToken = (Token_T)DynArray_get(oTokens, iStdinFlag);
      /* Quoted file name added without check. */
      if (Lex_returnType(psToken) == TOKEN_QUOTE)
      {
         psCmd = Syn_makeCmd(CMD_STDIN, Lex_returnValue(psToken));
         if (psCmd == NULL)
         {
            fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
            return FALSE;
         }
         if (! DynArray_add(oCmds, psCmd))
         {
            fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
            return FALSE;
         }
      }
      /* Unquoted invalid file names. */
      else if ((strcmp(Lex_returnValue(psToken), "<") == 0) ||
               (strcmp(Lex_returnValue(psToken), ">") == 0))
      {
         fprintf(stderr, 
                 "%s: standard input redirection without file name\n", 
                 pcProgName);
         return FALSE;
      }
      /* Add stdin redirection. */
      else
      {
         psCmd = Syn_makeCmd(CMD_STDIN, Lex_returnValue(psToken));
         if (psCmd == NULL)
         {
            fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
            return FALSE;
         }
         if (! DynArray_add(oCmds, psCmd))
         {
            fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
            return FALSE;
         }
      }  
         
   }

   /* Check stdout redirection. */
   /* No stdout redirection. */
   if (!iStdoutFlag)
   {
      psCmd = Syn_makeCmd(CMD_STDOUT, NULL);
      if (psCmd == NULL)
      {
         fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
         return FALSE;
      }
      if (! DynArray_add(oCmds, psCmd))
      {
         fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
         return FALSE;
      }
   }
   /* If there is stdout redirection. */
   else
   {
      /* stdout redirection without file name. */
      if (iStdoutFlag >= iLength)
      {
         fprintf(stderr, 
                 "%s: standard output redirection without file name\n", 
                 pcProgName);
         return FALSE;
      }
      psToken = (Token_T)DynArray_get(oTokens, iStdoutFlag);
      /* Quoted file name added without check. */
      if (Lex_returnType(psToken) == TOKEN_QUOTE)
      {
         psCmd = Syn_makeCmd(CMD_STDOUT, Lex_returnValue(psToken));
         if (psCmd == NULL)
         {
            fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
            return FALSE;
         }
         if (! DynArray_add(oCmds, psCmd))
         {
            fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
            return FALSE;
         }
      }
      /* Unquoted invalid file names. */
      else if ((strcmp(Lex_returnValue(psToken), "<") == 0) ||
               (strcmp(Lex_returnValue(psToken), ">") == 0))
      {
         fprintf(stderr, 
                 "%s: standard output redirection without file name\n", 
                 pcProgName);
         return FALSE;
      }
      /* Add stdout redirection. */
      else
      {
         psCmd = Syn_makeCmd(CMD_STDOUT, Lex_returnValue(psToken));
         if (psCmd == NULL)
         {
            fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
            return FALSE;
         }
         if (! DynArray_add(oCmds, psCmd))
         {
            fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
            return FALSE;
         }
      }           
   }
   return TRUE;
}

/*--------------------------------------------------------------------*/
