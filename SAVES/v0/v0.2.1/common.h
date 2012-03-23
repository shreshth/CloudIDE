#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED 1

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "dynarray.h"
#include "lex.h"
#include "syn.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILURE
#define FAILURE -1
#endif

#ifndef MAX_BUFF
#define MAX_BUFF 4096
#endif

#ifndef MAX_LINE_SIZE
#define MAX_LINE_SIZE 1024
#endif

#ifndef SERV_PORT
#define SERV_PORT 21002
#endif

#ifndef PERMISSIONS
#define PERMISSIONS 0600
#endif

#ifndef LONG_WIDTH
#define LONG_WIDTH 10
#endif

#define CMDNAME_COMPILE "compile"
#define CMDNAME_REMOTE "remote"
#define CMDNAME_RUN "runprog"
#define CMDNAME_SEND "sendfile"
#define CMDNAME_RECV "recvfile"
#define CMDNAME_REMOTE "remote"

#define EMPTYFILE "empty.txt"

/* function declarations */
void Common_makeSrcName(int iSockFD, char *pcName); /* make a source filename */
void Common_makeExecName(int iSockFD, char *pcName); /* make an executable filename */
void Common_makeOutName(int iSockFD, char *pcName); /* make an output filename */
void Common_itoa(int iConv, char *pcStr); /* convert int to string */
void Common_ltoa(long lConv, char *pcStr); /* convert long to string */
ssize_t Common_writen(int iFD, const void *pvBuf, size_t iSize); /* Write "n" bytes to a descriptor. */
ssize_t Common_readn(int iFD, void *pvBuf, size_t iSize); /* Read "n" bytes from a descriptor. */
int Common_sendFile(int iSockFD, char *pcSource); /* send a file through a file descriptor */
int Common_recvFile(int iSockFD, char *pcDest); /* receive a file through a file descriptor. if pcDest is NULL, write to stdout. */
void Common_checkSigUnblock(int signum); /* check that a signal is unblocked */
void Common_cleanup(DynArray_T oTokens, DynArray_T oCmds); /* free oTokens and oCmds arrays. */
char **Common_createArgv(DynArray_T oCmds); /* create argv array corresponding to oCmds and return pointer to it */
int Common_redirectStdin(DynArray_T oCmds, char *pcProgName); /* redirect stdin based on oCmds. */
int Common_redirectStdout(DynArray_T oCmds, char *pcProgName); /* redirect stdout based on oCmds. */
int Common_redirectStdoutForce(char *pcFileName, char *pcProgName); /* redirect stdout based to a filename. */
void Common_exec(DynArray_T oCmds, char *pcFileName, char *pcProgName); /* execute a command stored in oCmds. Redirect stdout depending on pcFileName. */

#endif
