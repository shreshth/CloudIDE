#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>

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

/* function declarations */
void Common_makeSrcName(int iSockFD, char *pcName); /* make a source filename */
void Common_makeExecName(int iSockFD, char *pcName); /* make an executable filename */
void Common_itoa(int iConv, char *pcStr); /* convert int to string */
ssize_t Common_writen(int iFD, const void *pvBuf, size_t iSize); /* Write "n" bytes to a descriptor. */
ssize_t Common_readn(int iFD, void *pvBuf, size_t iSize); /* Read "n" bytes from a descriptor. */
int Common_sendFile(int iSockFD, char *pcSource); /* send a file through a file descriptor */
int Common_recvFile(int iSockFD, char *pcDest); /* receive a file through a file descriptor */

