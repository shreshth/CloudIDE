#include "common.h"
       
/*--------------------------------------------------------------------*/     

/* make a source filename */
void Common_makeSrcName(int iSockFD, char *pcName)
{
  char acTemp[LONG_WIDTH];
  bzero(acTemp, LONG_WIDTH);
  Common_itoa(iSockFD, acTemp);
  strcat(pcName, "srv");
  strcat(pcName, acTemp);
  strcat(pcName, ".c");
}

/*--------------------------------------------------------------------*/     

/* make a executable filename */
void Common_makeExecName(int iSockFD, char *pcName)
{
  char acTemp[LONG_WIDTH];
  bzero(acTemp, LONG_WIDTH);
  Common_itoa(iSockFD, acTemp);
  strcat(pcName, "srv");
  strcat(pcName, acTemp);
}

/*--------------------------------------------------------------------*/     

/* make a executable filename */
void Common_makeOutName(int iSockFD, char *pcName)
{
  char acTemp[LONG_WIDTH];
  bzero(acTemp, LONG_WIDTH);
  Common_itoa(iSockFD, acTemp);
  strcat(pcName, "srv");
  strcat(pcName, acTemp);
  strcat(pcName, ".out");
}

/*--------------------------------------------------------------------*/     

/* convert int to string */
void Common_itoa(int iConv, char *pcStr)
{
  sprintf(pcStr, "%d", iConv);
}

/*--------------------------------------------------------------------*/     

/* convert long to string */
void Common_ltoa(long lConv, char *pcStr)
{
  sprintf(pcStr, "%ld", lConv);
}

/*--------------------------------------------------------------------*/     

/* Write "iSize" bytes to a descriptor. 
   NOTE: waits till iSize bytes are available. Be sure to check 
   the size you want to send.
 */
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
    //printf("writen: "); // DEBUG
    //fwrite(pcSave, iNWritten, 1, stdout); fflush(NULL); //DEBUG
    iNLeft -= iNWritten;
    pcSave += iNWritten;
  }
  return iSize;
}

/*--------------------------------------------------------------------*/     

/* Read "iSize" bytes from a descriptor. 
   NOTE: waits till iSize bytes are available. Be sure to check 
   the size you want to receive.
*/
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

/*--------------------------------------------------------------------*/     

/* send a file through a file descriptor */
/* POSSIBLE TO DO: if pcSource is NULL, read from stdin? Doesn't seem to
   be very useful, or make much sense */
int Common_sendFile(int iSockFD, char *pcSource)
{
  /* variable declarations and initializations */
  char acBuf[MAX_BUFF];
  FILE *FD = NULL;
  int iN = 0;
  int iWritten = 0;
  bzero(acBuf, MAX_BUFF);

  /* open source file to send */
  if (!(FD  = fopen(pcSource, "r"))) {
    perror("cannot open file");
    return FAILURE;
  }

  /* send size of file to server */
  fseek(FD, 0, SEEK_END);
  Common_ltoa(ftell(FD), acBuf);
  while (strlen(acBuf) != LONG_WIDTH + 1) acBuf[strlen(acBuf)] = '\n';
  Common_writen(iSockFD, acBuf, strlen(acBuf));
  fseek(FD, 0, SEEK_SET);
  bzero(acBuf, MAX_BUFF);
 
  /* send file to server */
  while ((iN = fread(acBuf, 1, MAX_BUFF, FD)) > 0) {
    //printf("\n*****\nWriting from %s:\n"/*%s*/, pcSource/*, acBuf*/); fflush(NULL); // DEBUG
    if ((iWritten = Common_writen(iSockFD, acBuf, iN)) == FAILURE) {
      fprintf(stderr, "error writing: %s\n", strerror(errno));
      return FAILURE;
    }
    assert(iWritten == iN);
    bzero(acBuf, MAX_BUFF);
  }

  /* if not reached end of file */
  if (feof(FD) == 0) {
    perror("could not send file entirely\n");
    return FAILURE;
  }

  fclose(FD);
  return SUCCESS;
}

/*--------------------------------------------------------------------*/            
/* receive a file through a file descriptor 
   if pcDest is NULL, write to stdout.
 */
int Common_recvFile(int iSockFD, char *pcDest)
{
  /* variable declarations */
  char acBuf[MAX_BUFF];
  char acCopy[MAX_BUFF];
  FILE *FD = NULL;
  ssize_t iGot = 0;
  long lFileLength = 0;
  int iIndex = 0;
  bzero(acBuf, MAX_BUFF);
  bzero(acCopy, MAX_BUFF);
  
  /* open destination file to receive */
  if (pcDest != NULL) {
    if (!(FD  = fopen(pcDest, "w"))) {
      perror("cannot open file");
      return FAILURE;
    }
  }

  /* recv size of file */
  iGot = Common_readn(iSockFD, acBuf, LONG_WIDTH + 1);
  if (iGot < 0) { /* error */
    fprintf(stderr, "error reading from socket\n");
    return FAILURE;
  }
  while (acBuf[iIndex] != '\n') iIndex++;
  memcpy(acCopy, acBuf, iIndex);
  lFileLength = atol(acCopy);

  /* read data from client and write to file */
  while (lFileLength > 0) {
    iGot = Common_readn(iSockFD, acBuf, 
			(lFileLength >= MAX_BUFF ? MAX_BUFF : lFileLength));
    //printf("*****\nReading to %s:\n%s", pcDest, acBuf); fflush(NULL); // DEBUG
    if (iGot < 0) { /* error */
      fprintf(stderr, "error reading from socket\n");
      return FAILURE;
    }
    
    if (iGot == 0) /* EOF */
      break;
    
    if (fwrite(acBuf, 1, iGot, (pcDest != NULL ? FD : stdout)) < (size_t) iGot) { /* otherwise write data to file */
      perror("error writing to file");
      return FAILURE;
    }

    bzero(acBuf, MAX_BUFF);
    lFileLength -= iGot;

    if (iGot < MAX_BUFF) /* EOF */
      break;  
  }
  
  assert(lFileLength == 0);

  if (pcDest != NULL) fclose(FD);
  return SUCCESS;
}

/*--------------------------------------------------------------------*/     

/* check that a signal is unblocked */
void Common_checkSigUnblock(int signum) 
{
  int iRet;
  sigset_t sSet;
  sigemptyset(&sSet);
  sigaddset(&sSet, signum);
  iRet = sigprocmask(SIG_UNBLOCK, &sSet, NULL);
  assert(iRet == 0);
}

/*--------------------------------------------------------------------*/     
/* free oTokens and oCmds arrays. */
void Common_cleanup(DynArray_T oTokens, DynArray_T oCmds)
{
   DynArray_map(oTokens, Lex_freeToken, NULL);
   DynArray_free(oTokens);
   DynArray_map(oCmds, Syn_freeCmd, NULL);
   DynArray_free(oCmds);
}

/*--------------------------------------------------------------------*/ 

/* create argv array corresponding to oCmds and return pointer to it */
char **Common_createArgv(DynArray_T oCmds)
{
   char **apcArgv = NULL;
   Cmd_T psCmd = NULL;
   int iArgs = 0;
   int i;

   assert(oCmds != NULL);

   /* Number of arguments in command. */
   for (i = 0; i < DynArray_getLength(oCmds); i++)
   {
      psCmd = (Cmd_T)DynArray_get(oCmds, i);
      if (Syn_returnType(psCmd) == CMD_ARG)
         iArgs++;
   }
   
   /* Create apcArgv array, of size iArgs + 2 (to hold command 
      name, arguments and NULL terminator). */
   apcArgv = (char**)calloc((size_t)(iArgs + 2), sizeof(char*));
   if (apcArgv == NULL)
      return NULL;
   
   /* Assign values to apcArgv elements. */
   for (i = 0, iArgs = 0; i < DynArray_getLength(oCmds); i++)
   {
      psCmd = (Cmd_T)DynArray_get(oCmds, i);
      /* Command arguments. */
      if (Syn_returnType(psCmd) == CMD_ARG)
         apcArgv[iArgs++] = Syn_returnValue(psCmd);
   }
   apcArgv[iArgs] = NULL;
 
   return apcArgv;
}

/*--------------------------------------------------------------------*/ 

/* redirect stdin based on oCmds. Function requires calling program's
   name for error-checking. Returns 1 (TRUE) if successful, or 0 (FALSE)
   if an error occured. */
int Common_redirectStdin(DynArray_T oCmds, char *pcProgName) 
{
   int i;
   int iFd;
   int iRet;
   int iErrSv;
   Cmd_T psCmd = NULL;
   char *pcFileName = NULL;

   assert(oCmds != NULL);
   assert(pcProgName != NULL);

   /* Check for stdin redirection. */
   for (i = 0; i < DynArray_getLength(oCmds); i++)
   {
      psCmd = (Cmd_T)DynArray_get(oCmds, i);
      /* Redirect stdin, if required. */
      if (Syn_returnType(psCmd) == CMD_STDIN)
      {
         pcFileName = Syn_returnValue(psCmd);
         if (pcFileName != NULL)
         {
            /* Create new file descriptor. */
            iFd = open(pcFileName, O_RDONLY);
            if (iFd == -1) 
            {
               iErrSv = errno; 
               fprintf(stderr, "%s: %s: %s\n", pcProgName,  pcFileName, 
                       strerror(iErrSv));
               return FALSE; 
            }
            
            /* Close stdin file descriptor. */
            iRet = close(0);
            if (iRet == -1) 
            {
               perror(pcProgName);
               return FALSE; 
            }

            /* Duplicate new file descriptor to stdin. */
            iRet = dup(iFd);
            if (iRet == -1) 
           { 
              perror(pcProgName);
               return FALSE; 
            }            
            /* Close the file descriptor created in first step. */
            iRet = close(iFd);
            if (iRet == -1) 
            {
               perror(pcProgName);
               return FALSE; 
            }
         }
         return TRUE;
      }
   }
   return TRUE;
}

/*--------------------------------------------------------------------*/ 

/* redirect stdout based on oCmds. Function requires calling program's
   name for error-checking. Returns 1 (TRUE) if successful, or 0 (FALSE)
   if an error occured. */
int Common_redirectStdout(DynArray_T oCmds, char *pcProgName)   
{
   int i;
   int iFd;
   int iRet;
   int iErrSv;
   Cmd_T psCmd = NULL;
   char *pcFileName = NULL;
   
   assert(oCmds != NULL);
   assert(pcProgName != NULL);

  /* Check for stdout redirection. */
   for (i = 0; i < DynArray_getLength(oCmds); i++)
   {
      psCmd = (Cmd_T)DynArray_get(oCmds, i);
      /* Redirect stdout, if required. */
      if (Syn_returnType(psCmd) == CMD_STDOUT)
      {
         pcFileName = Syn_returnValue(psCmd);
         if (pcFileName != NULL)
         {
            /* Create new file descriptor. */
            iFd = creat(pcFileName, PERMISSIONS);
            if (iFd == -1) 
            {
               iErrSv = errno; 
               fprintf(stderr, "%s: %s: %s\n", pcProgName,  pcFileName, 
                       strerror(iErrSv));
               return FALSE; 
            }

            /* Close stdout file descriptor. */
            iRet = close(1);
            if (iRet == -1) 
            {
               perror(pcProgName);
               return FALSE; 
            }
            
            /* Duplicate new file descriptor to stdout. */
            iRet = dup(iFd);
            if (iRet == -1) 
            {
               perror(pcProgName);
               return FALSE; 
            }
            
            /* Close the file descriptor created in first step. */
            iRet = close(iFd);
            if (iRet == -1) 
            {
               perror(pcProgName);
               return FALSE; 
            }
         }
         return TRUE;
      }
   }
   return TRUE;
}

/*--------------------------------------------------------------------*/ 

/* redirect stdout to a filename. 
   Returns 1 (TRUE) if successful, or 0 (FALSE) if an error occured. 
*/
int Common_redirectStdoutForce(char *pcFileName, char *pcProgName)   
{
  int iFd;
  int iRet;
  int iErrSv;
  
  assert(pcFileName != NULL);
  assert(pcProgName != NULL);
  
  //printf("\nRedirecting stdout to: %s\n", pcFileName); fflush(NULL); //DEBUG
  
  /* Create new file descriptor. */
  iFd = creat(pcFileName, PERMISSIONS);
  if (iFd == -1) {
    iErrSv = errno; 
    fprintf(stderr, "%s: %s: %s\n", pcProgName,  pcFileName, 
	    strerror(iErrSv));
    return FALSE; 
  }
  
  /* Close stdout file descriptor. */
  iRet = close(1);
  if (iRet == -1) {
    perror(pcProgName);
    return FALSE; 
  }
  
  /* Duplicate new file descriptor to stdout. */
  iRet = dup(iFd);
  if (iRet == -1) {
    perror(pcProgName);
    return FALSE; 
  }
  
  /* Close the file descriptor created in first step. */
  iRet = close(iFd);
  if (iRet == -1) {
    perror(pcProgName);
    return FALSE; 
  }

  //printf("TEST"); // DEBUG

  return TRUE;
}

/*--------------------------------------------------------------------*/ 

/* execute a command stored in oCmds. Redirect stdout depending on
   pcFileName
*/
void Common_exec(DynArray_T oCmds, char *pcFileName, char *pcProgName)
{
  int iPid = 0;
  int iErrSv = 0;
  char **apcArgv = NULL;
  //int i = 0; // DEBUG

  //printf("\nCOMMAND:"); // DEBUG
  //for (i = 0; i < DynArray_getLength(oCmds); i++) // DEBUG
      //printf("\n%d: %s", i, (char *) Syn_returnValue((Cmd_T) DynArray_get(oCmds, i))); // DEBUG
  
  /* create arguments array */
  if ((apcArgv = Common_createArgv(oCmds)) == NULL) {
    fprintf(stderr, "%s: cannot create argv array\n", pcProgName);
    return;
  }
  //printf("\nARGV:"); // DEBUG
  //for (i = 0; i < DynArray_getLength(oCmds); i++) // DEBUG
  //printf("\n%d: %s", i, apcArgv[i]); // DEBUG


  /* fork */
  fflush(NULL);
  if ((iPid = fork()) == -1) {
    perror(pcProgName);
    return;
  }

  /* child process */
  if (iPid == 0) {
    /* Redirect stdin */
    if(!Common_redirectStdin(oCmds, pcProgName))
      exit(EXIT_FAILURE);

    /* redirect stdout */
    if (pcFileName == NULL) {
      if(!Common_redirectStdout(oCmds, pcProgName))
	exit(EXIT_FAILURE);
    }
    else {
      if (!Common_redirectStdoutForce(pcFileName, pcProgName))
	exit(EXIT_FAILURE);
    }
    
    /* Execute command. */
    execvp(apcArgv[0], apcArgv);
    iErrSv = errno;
    fprintf(stderr, "%s: %s\n", pcProgName, strerror(iErrSv));
    exit(EXIT_FAILURE);
  }

  /* parent process */
  if ((iPid = wait(NULL)) == -1) {
    perror(pcProgName);
    return;
  }

  /* cleanup */
  free(apcArgv);
}

/*--------------------------------------------------------------------*/ 
