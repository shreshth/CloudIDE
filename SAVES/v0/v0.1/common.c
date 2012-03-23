#include "common.h"

/* make a source filename */
void Common_makeSrcName(int iSockFD, char *pcName)
{
  char acTemp[10];
  Common_itoa(iSockFD, acTemp);
  strcat(pcName, "srv");
  strcat(pcName, acTemp);
  strcat(pcName, ".c");
}

/* make a executable filename */
void Common_makeExecName(int iSockFD, char *pcName)
{
  char acTemp[10];
  Common_itoa(iSockFD, acTemp);
  strcat(pcName, "srv");
  strcat(pcName, acTemp);
}

/* convert int to string */
void Common_itoa(int iConv, char *pcStr)
{
  int i = 0;
  while (iConv > 0) {
    pcStr[i++] = (char) (iConv % 10);
    iConv /= 10;
  }
  pcStr[i] = '\0';
}

/* Write "n" bytes to a descriptor. */
ssize_t Common_writen(int iFD, const void *pvBuf, size_t iSize)
{
  /* variable declarations and initializations */
  size_t iNLeft;
  ssize_t iNWritten;
  const char *pcSave;
  pcSave = (char *) pvBuf;
  iNLeft = iSize;
  
  /* write */
  while (iNLeft > 0) {
    if ((iNWritten = write(iFD, pcSave, iNLeft)) <= 0) {
      if ((iNWritten < 0) && (errno == EINTR)) {
	iNWritten = 0;   /* and call write() again */
      }
      else {
	fprintf(stderr, "error: Common_writen()\n");
	return FAILURE;    /* error */
      }
    }
    
    iNLeft -= iNWritten;
    pcSave += iNWritten;
  }
  return iSize;
}

/* Read "n" bytes from a descriptor. */
ssize_t Common_readn(int iFD, void *pvBuf, size_t iSize)
{
  /* variable declarations and initializations */
  size_t iNLeft;
  ssize_t iNRead;
  char *pcSave;  
  pcSave = (char *) pvBuf;
  iNLeft = iSize;

  /* read */
  while (iNLeft > 0) {
    if ((iNRead = read(iFD, pcSave, iNLeft)) < 0) {
      if (errno == EINTR) {
	iNRead = 0;      /* and call read() again */
      }
      else {
	return FAILURE;
      }
    } 
    else if (iNRead == 0) {
      break;              /* EOF */
    }
    
    iNLeft -= iNRead;
    pcSave += iNRead;
  }
  return (iSize - iNLeft);         
}

/* send a file through a file descriptor */
int Common_sendFile(int iSockFD, char *pcSource)
{
  /* variable declarations and initializations */
  char acBuf[MAX_BUFF];
  FILE *FD = NULL;
  int iN = 0;
  int iWritten = 0;

  /* open source file to send */
  if (!(FD  = fopen(pcSource, "r"))) {
    perror("cannot open file");
    return FAILURE;
  }

  printf("%s\n\n", pcSource); fflush(NULL); // DEBUG

  /* send file to server */
  while ((iN = fread(acBuf, 1, MAX_BUFF, FD))) {
    printf("Sending: %s", acBuf); fflush(NULL); // DEBUG
    iWritten = Common_writen(iSockFD, acBuf, iN);
    if (iWritten < 0) {
      fprintf(stderr, "error writing %s\n", strerror(errno));
      return FAILURE;
    }
  }
  
  /* if not reached end of file */
  if (!feof(FD)) {
    perror("could not send file entirely\n");
    return FAILURE;
  }

  fclose(FD);
  return SUCCESS;
}

/* receive a file through a file descriptor */
int Common_recvFile(int iSockFD, char *pcDest)
{
  /* variable declarations */
  char acBuf[MAX_BUFF];
  FILE *FD = NULL;
  ssize_t iGot = 0;
  
  /* open destination file to receive */
  if (!(FD  = fopen(pcDest, "w"))) {
    perror("cannot open file");
    return FAILURE;
  }

  /* read data from client and write to file */
  while (TRUE) {
    iGot = Common_readn(iSockFD, acBuf, MAX_BUFF);
    if (iGot < 0) { /* error */
      fprintf(stderr, "error reading from socket\n");
      return FAILURE;
    }
    
    if (iGot == 0) /* EOF */
      break;
    
    if (fwrite(acBuf, 1, iGot, FD) < (size_t) iGot) { /* otherwise write data to file */
      perror("error writing to file");
      return FAILURE;
    }
    
    if (iGot < MAX_BUFF) /* EOF */
      break;  
  };
  
  fclose(FD);
  return SUCCESS;
}


