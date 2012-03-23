/* TO DOS:
   1. Delete srvX.out after every command
   2. Change stdout redirection after cd
*/

#include "server.h"

/*--------------------------------------------------------------------*/

static void Server_executeCommand(char *acLine, DynArray_T oTokens, DynArray_T oCmds, int iSockFD); /* execute a command contained in acLine */
static int Server_handleSend(DynArray_T oCmds, int iSockFD); /* receive a file from remote client */
static int Server_handleRecv(DynArray_T oCmds, int iSockFD); /* send a file to remote client */
static void Server_exec(DynArray_T oCmds, int iSockFD); /* execute command stored in oCmds, and send terminal result back to client */
static int Server_recvCommand(int iSockFD, char *acLine); /* receive a command from remote client */

/*--------------------------------------------------------------------*/

int main(int argc, char **argv)
{
  /* variable declarations and initializations */
  int iListenFD = 0, iConnFD = 0;
  char acLine[MAX_LINE_SIZE];
  DynArray_T oTokens = NULL;
  DynArray_T oCmds = NULL;
  pid_t iChildPID = 0;
  socklen_t iCliLen = 0;
  struct sockaddr_in sCliAddr, sServAddr;
  bzero(&sCliAddr, sizeof(sCliAddr));
  bzero(&sServAddr, sizeof(sServAddr));
  
  /* check usage */
  if (argc != 1) {
    printf("usage: server\n");
    exit(EXIT_FAILURE);
  }
  
  /* create socket */
  if ((iListenFD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("server: socket");
    exit(EXIT_FAILURE);
  }
  
  /* address structure */
  bzero(&sServAddr, sizeof(sServAddr));
  sServAddr.sin_family = AF_INET;
  sServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  sServAddr.sin_port = htons(SERV_PORT);
  
  /* bind connection */
  if (bind(iListenFD, (struct sockaddr *) &sServAddr, sizeof(sServAddr)) < 0) {
    perror("server: bind");
    exit(EXIT_FAILURE);
  }
  
  /* listen for incoming connections */
  listen(iListenFD, MAX_PENDING);
  
  /* get new connections and start new processes for them */
  while (TRUE) {
    iCliLen = sizeof(sCliAddr);
    iConnFD = accept(iListenFD, (struct sockaddr *) &sCliAddr, &iCliLen);
    
    if ((iChildPID = fork()) == 0) { /* child process */
      close(iListenFD);              /* close listening socket */
      
      /*****************************************************************
       ********** At this point, client is connected to server *********
       *****************************************************************/

      while (Server_recvCommand(iConnFD, acLine) == SUCCESS) {
	if (strlen(acLine)) {
	  Server_executeCommand(acLine, oTokens, oCmds, iConnFD);
	}
	bzero(acLine, MAX_LINE_SIZE);
      }	
      close(iConnFD);
      exit(EXIT_SUCCESS);
    }
    close(iConnFD); /* parent closes connected socket */
  }
}

/*--------------------------------------------------------------------*/

/* execute a command contained in acLine */
static void Server_executeCommand(char *acLine, DynArray_T oTokens,
				  DynArray_T oCmds, int iSockFD)
{
  int iSuccessful = 0;
  Cmd_T psCmd;
  char acOutName[MAX_NAME];
  bzero(acOutName, MAX_NAME);
 
  
  assert(acLine != NULL);

  /* redirect stdout and stderr */
  Common_makeOutName(iSockFD, acOutName);
  Common_redirectStdoutForce(acOutName, "server");
  Common_redirectStderrForce(acOutName, "server");

  /* create empty DynArrays */
  if ((oTokens = DynArray_new(0)) == NULL) {
    fprintf(stderr, "server: Server_executeCommand: cannot allocate memory\n");
    Common_deleteFile(acOutName, "server");
    return;
  }
  if ((oCmds = DynArray_new(0)) == NULL) {
    fprintf(stderr, "server: Server_executeCommand: cannot allocate memory\n");
    Common_deleteFile(acOutName, "server");
    return;
  }
  
  /* lexical analysis stage. */
  iSuccessful = Lex_lexLine(acLine, oTokens, "server");
  /* check if successful lexical analysis, and if token
     array size is greater than zero. */
  if ((!iSuccessful) || (!DynArray_getLength(oTokens))) {  
    Common_cleanup(oTokens, oCmds);
    Common_deleteFile(acOutName, "server");
    return;
  }
  
  /* syntactical parsing stage. */
  iSuccessful = Syn_synLine(oTokens, oCmds, "server");
  /* check if successful syntactical parsing. */
  if (!iSuccessful) {  
    Common_cleanup(oTokens, oCmds);
    Common_deleteFile(acOutName, "server");
    return;
  }  
  
  /* check for custom commands */
  if (Server_handleSend(oCmds, iSockFD)) { /* receive a file from remote client */
    Common_cleanup(oTokens, oCmds);
    Common_deleteFile(acOutName, "server");
    return;
  }
  else if (Server_handleRecv(oCmds, iSockFD)) { /* send a file to remote client */
    Common_cleanup(oTokens, oCmds);
    Common_deleteFile(acOutName, "server");
    return;
  }

  /* remove remote keyword */
  psCmd = (Cmd_T) DynArray_get(oCmds, 0);
  assert(Syn_returnType(psCmd) == CMD_CMD);
  assert(strcmp(Syn_returnValue(psCmd), CMDNAME_REMOTE) == 0);
  DynArray_removeAt(oCmds, 0);
  
  /* execute all other commands */
  Server_exec(oCmds, iSockFD);

  /* cleanup */
  Common_deleteFile(acOutName, "server");
  Common_cleanup(oTokens, oCmds);
}

/*--------------------------------------------------------------------*/

/* receive a file from remote client */
static int Server_handleSend(DynArray_T oCmds, int iSockFD)
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

  /* receive file from client */
  psCmd = (Cmd_T) DynArray_get(oCmds, 1);

  assert(Common_recvFile(iSockFD, Syn_returnValue(psCmd)) == SUCCESS);
  
  return TRUE;
}

/*--------------------------------------------------------------------*/

/* send a file to remote client */
static int Server_handleRecv(DynArray_T oCmds, int iSockFD)
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

  /* if not recv */
  if (!iFlag) return FALSE;

  /* send file to client */
  psCmd = (Cmd_T) DynArray_get(oCmds, 1);
  if (Common_sendFile(iSockFD, Syn_returnValue(psCmd)) == FAILURE) {
    assert(Common_sendFile(iSockFD, EMPTYFILE) == SUCCESS);
  }

  return TRUE;
}
  
/*--------------------------------------------------------------------*/

/* execute command stored in oCmds, and send terminal result back to client */
static void Server_exec(DynArray_T oCmds, int iSockFD)
{
  char *pcPathSave = NULL;
  char *pcPath = NULL;
  char acOutName[MAX_NAME];
  bzero(acOutName, MAX_NAME);

  assert(oCmds != NULL);
  
  if (strcmp((char*) Syn_returnValue((Cmd_T) DynArray_get(oCmds, 0)), "cd") == 0) {
    Common_makeOutName(iSockFD, acOutName);
    pcPathSave = getcwd(NULL, 0);
    if (Common_handleCd(oCmds, "server")) /* change current directory */
      { 
	/* delete previous random text file */
	pcPath = (char *) calloc(1, strlen(pcPathSave) + strlen(acOutName) + 2);
	strcat(pcPath, pcPathSave);
	strcat(pcPath, "/");
	strcat(pcPath, acOutName);
	//printf("\nPath to delete: %s\n", pcPath, ); // DEBUG
	Common_deleteFile(pcPath, "server");

	/* redirect stdout */
	Common_redirectStdoutForce(acOutName, "server");
	Common_redirectStderrForce(acOutName, "server");
	assert(Common_sendFile(iSockFD, acOutName) == SUCCESS);
      }
  }
  else if (Common_handleSetenv(oCmds, "server")) /* set environment variable value */
    { 
      Common_makeOutName(iSockFD, acOutName);
      assert(Common_sendFile(iSockFD, acOutName) == SUCCESS);
    }
  else if (Common_handleUnsetenv(oCmds, "server")) /* unset environment variable value */
    { 
      Common_makeOutName(iSockFD, acOutName);
      assert(Common_sendFile(iSockFD, acOutName) == SUCCESS);
    }
  else if (strcmp((char*) Syn_returnValue((Cmd_T) DynArray_get(oCmds, 0)), "exit") == 0) { /* exit program */
    Common_makeOutName(iSockFD, acOutName);
    Common_deleteFile(acOutName, "server");
    assert(Common_handleExit(oCmds, "server")); /* exit program */
  }
  else 
    {
      Common_exec(oCmds, "server"); 
      Common_makeOutName(iSockFD, acOutName);
      assert(Common_sendFile(iSockFD, acOutName) == SUCCESS);
    }
}

/*--------------------------------------------------------------------*/

/* receive a command from remote client */
static int Server_recvCommand(int iSockFD, char *acLine)
{
  int i = 0;
  int iRead = 0;

  while (i < MAX_LINE_SIZE - 1) {
    iRead = recv(iSockFD, acLine + i, 1, 0);
    
    if (iRead < 0) {
      perror("error reading command");
      return FAILURE;
    }
    
    assert(iRead == 1);
    //printf("%d: %c\n", i, acLine[i]); fflush(NULL); // DEBUG

    if (acLine[i] == '\n') {
      break;
    }
    i++;
  }
  acLine[i] = '\0';
  return SUCCESS;
}
