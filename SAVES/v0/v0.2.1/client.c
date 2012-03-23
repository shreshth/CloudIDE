#include "client.h"

/*--------------------------------------------------------------------*/

static void Client_executeCommand(char *acLine, DynArray_T oTokens, DynArray_T oCmds, int iSockFD); /* execute a command contained in acLine */
static int Client_handleCompile(DynArray_T oCmds, int iSockFD, char *acLine); /* compile a remote file */
static int Client_handleRun(DynArray_T oCmds, int iSockFD, char *acLine); /* run a remote file */
static int Client_handleSend(DynArray_T oCmds, int iSockFD, char *acLine); /* send a file to remote server */
static int Client_handleRecv(DynArray_T oCmds, int iSockFD, char *acLine); /* receive a file from remote server */
static int Client_handleRemote(DynArray_T oCmds, int iSockFD, char *acLine); /* any other remote command */
static void Client_recvResponse(int iSockFD); /* receive response from socket and print to stdout */

/*--------------------------------------------------------------------*/

int main(int argc, char **argv)
{
  /* variable declarations and initializations */
  char acPrompt[2] = "%";
  char acLine[MAX_LINE_SIZE];
  DynArray_T oTokens = NULL;
  DynArray_T oCmds = NULL;
  int iSockFD = 0;
  struct sockaddr_in sServAddr;
  bzero(&sServAddr, sizeof(sServAddr));
  
  /* check usage */
  if (argc != 2) {
    fprintf(stderr, "usage: client <server>\n");
    exit(-1);	      
  }
  
  /* create socket */
  if ((iSockFD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("client: socket");
    exit(EXIT_FAILURE);
  }
  
  /* address structure */
  bzero(&sServAddr, sizeof(sServAddr));
  sServAddr.sin_family = AF_INET;
  sServAddr.sin_port = htons(SERV_PORT);
  inet_pton(AF_INET, argv[1], &sServAddr.sin_addr);
  
  /* connect */
  if (connect(iSockFD, (struct sockaddr *) &sServAddr, sizeof(sServAddr)) < 0) {
    fprintf(stderr, "connect failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  /********************************************************************
   ********** At this point, client is connected to server ************
   ********************************************************************/

  printf("%s ", acPrompt);

  while (fgets(acLine, MAX_LINE_SIZE, stdin)) {
    if (strlen(acLine)) {
      Client_executeCommand(acLine, oTokens, oCmds, iSockFD);
    }
    bzero(acLine, MAX_LINE_SIZE);
    printf("%s ", acPrompt);
  }

  printf("\n");

  exit(EXIT_SUCCESS);
}

/*--------------------------------------------------------------------*/

/* execute a command contained in acLine */
static void Client_executeCommand(char *acLine, DynArray_T oTokens,
				  DynArray_T oCmds, int iSockFD)
{
  int iSuccessful = 0;
  
  assert(acLine != NULL);

  /* create empty DynArrays */
  if ((oTokens = DynArray_new(0)) == NULL) {
    fprintf(stderr, "client: Server_executeCommand: cannot allocate memory\n");
    return;
  }
  if ((oCmds = DynArray_new(0)) == NULL) {
    fprintf(stderr, "client: Server_executeCommand: cannot allocate memory\n");
    return;
  }
  
  /* lexical analysis stage. */
  iSuccessful = Lex_lexLine(acLine, oTokens, "client");
  /* check if successful lexical analysis, and if token
     array size is greater than zero. */
  if ((!iSuccessful) || (!DynArray_getLength(oTokens))) {  
    Common_cleanup(oTokens, oCmds);
    return;
  }
  
  /* syntactical parsing stage. */
  iSuccessful = Syn_synLine(oTokens, oCmds, "client");
  /* check if successful syntactical parsing. */
  if (!iSuccessful) {  
    Common_cleanup(oTokens, oCmds);
    return;
  }  
  
  /* check for custom commands */
  if (Client_handleCompile(oCmds, iSockFD, acLine)) { /* compile a remote file */
    Common_cleanup(oTokens, oCmds);
    return;
  }
  else if (Client_handleRun(oCmds, iSockFD, acLine)) { /* run a remote file */
    Common_cleanup(oTokens, oCmds);
    return;
  }
  else if (Client_handleSend(oCmds, iSockFD, acLine)) { /* send a file to remote server */
    Common_cleanup(oTokens, oCmds);
    return;
  }
  else if (Client_handleRecv(oCmds, iSockFD, acLine)) { /* receive a file from remote server */
    Common_cleanup(oTokens, oCmds);
    return;
  }
  else if (Client_handleRemote(oCmds, iSockFD, acLine)) { /* any other remote command */
    Common_cleanup(oTokens, oCmds);
    return;
  }

  /* execute other commands */
  Common_exec(oCmds, NULL, "client");
  Common_cleanup(oTokens, oCmds);
}

/*--------------------------------------------------------------------*/

/* compile a remote file */
static int Client_handleCompile(DynArray_T oCmds, int iSockFD, char *acLine)
{
  int iLength = 0;
  int i = 0;
  int iFlag = FALSE;
  Cmd_T psCmd = NULL;
  CmdType eType;
  int iArgs = 0;

  assert(oCmds != NULL);

  iLength = DynArray_getLength(oCmds);

  /* Go through entire command */
  for (i = 0; i < iLength; i++) {
    psCmd = (Cmd_T) DynArray_get(oCmds, i);
    eType = Syn_returnType(psCmd);
    if (eType == CMD_CMD) {
      if (strcmp(Syn_returnValue(psCmd), CMDNAME_COMPILE) == 0) {
	assert(i == 0); 
	iFlag = TRUE;
      }
      else break;
    }
    else if (eType == CMD_ARG) iArgs++;
  }

  /* if not compile */
  if (!iFlag) return FALSE;

  /* send command and receive response from server */
  assert(Common_writen(iSockFD, acLine, strlen(acLine)) != FAILURE);
  Client_recvResponse(iSockFD);

  return TRUE;
}

/*--------------------------------------------------------------------*/

/* run a remote file */
static int Client_handleRun(DynArray_T oCmds, int iSockFD, char *acLine)
{
  int iLength = 0;
  int i = 0;
  int iFlag = FALSE;
  Cmd_T psCmd = NULL;
  CmdType eType;
  int iArgs = 0;

  assert(oCmds != NULL);

  iLength = DynArray_getLength(oCmds);

  /* Go through entire command */
  for (i = 0; i < iLength; i++) {
    psCmd = (Cmd_T) DynArray_get(oCmds, i);
    eType = Syn_returnType(psCmd);
    if (eType == CMD_CMD) {
      if (strcmp(Syn_returnValue(psCmd), CMDNAME_RUN) == 0) {
	assert(i == 0); 
	iFlag = TRUE;
      }
      else break;
    }
    else if (eType == CMD_ARG) iArgs++;
  }

  /* if not run */
  if (!iFlag) return FALSE;

  /* send command and receive response from server */
  assert(Common_writen(iSockFD, acLine, strlen(acLine)) != FAILURE);
  Client_recvResponse(iSockFD);
 
  return TRUE;
}

/*--------------------------------------------------------------------*/

/* send a file to remote server */
static int Client_handleSend(DynArray_T oCmds, int iSockFD, char *acLine)
{
  int iLength = 0;
  int i = 0;
  int iFlag = FALSE;
  Cmd_T psCmd = NULL;
  CmdType eType;
  int iArgs = 0;

  assert(oCmds != NULL);

  iLength = DynArray_getLength(oCmds);

  /* Go through entire command */
  for (i = 0; i < iLength; i++) {
    psCmd = (Cmd_T) DynArray_get(oCmds, i);
    eType = Syn_returnType(psCmd);
    if (eType == CMD_CMD) {
      if (strcmp(Syn_returnValue(psCmd), CMDNAME_SEND) == 0) {
	assert(i == 0); 
	iFlag = TRUE;
      }
      else break;
    }
    else if (eType == CMD_ARG) iArgs++;
  }

  /* if not send */
  if (!iFlag) return FALSE;

  /* send file to server */
  assert(Common_writen(iSockFD, acLine, strlen(acLine)) != FAILURE);
  psCmd = (Cmd_T) DynArray_get(oCmds, 1);
  if (Common_sendFile(iSockFD, Syn_returnValue(psCmd)) == FAILURE) {
    assert(Common_sendFile(iSockFD, EMPTYFILE) == SUCCESS);
  }
  
  return TRUE;
}

/*--------------------------------------------------------------------*/

/* receive a file from remote server */
static int Client_handleRecv(DynArray_T oCmds, int iSockFD, char *acLine)
{
  int iLength = 0;
  int i = 0;
  int iFlag = FALSE;
  Cmd_T psCmd = NULL;
  CmdType eType;
  int iArgs = 0;

  assert(oCmds != NULL);

  iLength = DynArray_getLength(oCmds);

  /* Go through entire command */
  for (i = 0; i < iLength; i++) {
    psCmd = (Cmd_T) DynArray_get(oCmds, i);
    eType = Syn_returnType(psCmd);
    if (eType == CMD_CMD) {
      if (strcmp(Syn_returnValue(psCmd), CMDNAME_RECV) == 0) {
	assert(i == 0); 
	iFlag = TRUE;
      }
      else break;
    }
    else if (eType == CMD_ARG) iArgs++;
  }

  /* if not send */
  if (!iFlag) return FALSE;

  /* receive file from server */
  assert(Common_writen(iSockFD, acLine, strlen(acLine)) != FAILURE);
  psCmd = (Cmd_T) DynArray_get(oCmds, 1);
  assert(Common_recvFile(iSockFD, Syn_returnValue(psCmd)) == SUCCESS);
  
  return TRUE;
}

/*--------------------------------------------------------------------*/

/* any other remote command */
static int Client_handleRemote(DynArray_T oCmds, int iSockFD, char *acLine)
{
  int iLength = 0;
  int i = 0;
  int iFlag = FALSE;
  Cmd_T psCmd = NULL;
  CmdType eType;
  int iArgs = 0;

  assert(oCmds != NULL);

  iLength = DynArray_getLength(oCmds);

  /* Go through entire command */
  for (i = 0; i < iLength; i++) {
    psCmd = (Cmd_T) DynArray_get(oCmds, i);
    eType = Syn_returnType(psCmd);
    if (eType == CMD_CMD) {
      if (strcmp(Syn_returnValue(psCmd), CMDNAME_REMOTE) == 0) {
	assert(i == 0); 
	iFlag = TRUE;
      }
      else break;
    }
    else if (eType == CMD_ARG) iArgs++;
  }

  /* if not remote */
  if (!iFlag) return FALSE;

  /* send command and receive response from server */
  assert(Common_writen(iSockFD, acLine, strlen(acLine)) != FAILURE);
  Client_recvResponse(iSockFD);
 
  return TRUE; 
}

/*--------------------------------------------------------------------*/

/* receive response from socket and print to stdout */
static void Client_recvResponse(int iSockFD)
{
  /*
  char acBuf[MAX_BUFF];
  int iGot = 0;

  * read data from client and write to file *
  while (TRUE) {
    iGot = Common_readn(iSockFD, acBuf, MAX_BUFF);
    if (iGot < 0) { * error *
      fprintf(stderr, "error reading from socket\n");
      return;
    }
    
    if (iGot == 0) * EOF *
      break;

    * otherwise write data to stdout *
    printf("%s", acBuf);
    
    if (iGot < MAX_BUFF) * EOF *
      break;  
  };
  */
  assert(Common_recvFile(iSockFD, NULL) == SUCCESS);
  

}

/*--------------------------------------------------------------------*/
