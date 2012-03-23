#ifndef SERVER_INCLUDED
#define SERVER_INCLUDED 1

#include "common.h"

#ifndef MAX_PENDING
#define MAX_PENDING 128
#endif 

#ifndef MAX_NAME
#define MAX_NAME 20
#endif

/* function declarations */
int Server_compile(char *pcSrc, char *pcExec); /* compile file */


#endif
