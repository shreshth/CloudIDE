/*--------------------------------------------------------------------*/
/* syn.h                                                              */
/* Author: Shreshth Singhal (netID: ssinghal)                         */
/* Syntactical analyzer for interactive shell                         */
/*--------------------------------------------------------------------*/

#ifndef SYN_INCLUDED
#define SYN_INCLUDED

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

typedef struct Cmd *Cmd_T;
/* A command word is a string which is a part of a shell command
   along with its type */

enum CmdType {CMD_CMD, CMD_ARG, CMD_STDIN, CMD_STDOUT};
typedef enum CmdType CmdType;

/*--------------------------------------------------------------------*/

CmdType Syn_returnType(void *pvItem);
/* Return the command word's type. */

char *Syn_returnValue(void *pvItem);
/* Return the command word's value. */

void Syn_freeCmd(void *pvItem, void *pvExtra);
/* Free command word pvItem. pvExtra is unused. */

int Syn_synLine(DynArray_T oTokens, DynArray_T oCmds, char* pcProgName);
/* Syntactically analyze the token array oTokens. Populate oCmds
   with tokens that oTokens contains along with their type. 
   Return 1 (TRUE) if successful, or 0 (FALSE) otherwise. In the 
   latter case, oCmds may contain tokens that were discovered before 
   the error. The caller owns the tokens placed in oCmds. Note that
   the function needs the calling program's name for error-checking. */ 

#endif
