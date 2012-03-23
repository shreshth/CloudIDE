#include "server.h"

int main(int argc, char **argv)
{
  /* variable declarations and initializations */
  int iListenFD = 0, iConnFD = 0;
  pid_t iChildPID = 0;
  socklen_t iCliLen = 0;
  char acSrcName[MAX_NAME];
  char acExecName[MAX_NAME];
  struct sockaddr_in sCliAddr, sServAddr;
  bzero(&sCliAddr, sizeof(sCliAddr));
  bzero(&sServAddr, sizeof(sServAddr));
  
  /* check usage */
  if (argc != 2) {
    printf("usage: server <port>\n");
    exit(-1);
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
  sServAddr.sin_port = htons(atoi(argv[1]));
  
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
      
      
      /* get source and executable file names */
      bzero(acSrcName, MAX_NAME);
      Common_makeSrcName(iConnFD, acSrcName);
      bzero(acExecName, MAX_NAME);
      Common_makeExecName(iConnFD, acExecName);
      
      /* read file from client */
      if (Common_recvFile(iConnFD, acSrcName) == FAILURE) {
	close(iConnFD);
	exit(EXIT_FAILURE);
      }
      
      /* compile file from client */
      if (Server_compile(acSrcName, acExecName) == FAILURE) {
	close(iConnFD);
	exit(EXIT_FAILURE);
      }

      /* send file to client */
      if (Common_sendFile(iConnFD, acExecName) == FAILURE) {   
	close(iConnFD);
	exit(EXIT_FAILURE);
      }
      
      exit(EXIT_SUCCESS);
    }
    close(iConnFD); /* parent closes connected socket */
  }
}

/* compile a file */
int Server_compile(char *pcSrc, char *pcExec)
{
  /* variable declarations and initializations */
  char **ppcArgv = NULL;
  int iErrSv = 0, iPid = 0;
  int i = 0;

  /* Create arguments array. */
  if ((ppcArgv = calloc(5, sizeof(char *))) == NULL) {
    fprintf(stderr, "out of memory\n");
    return FAILURE;
  }
  ppcArgv[0] = calloc(1, 5); strcat(ppcArgv[0], "gcc");
  ppcArgv[1] = calloc(1, sizeof(pcSrc)); strcat(ppcArgv[1], pcSrc);
  ppcArgv[2] = calloc(1, 4); strcat(ppcArgv[2], "-o");
  ppcArgv[3] = calloc(1, sizeof(pcExec)); strcat(ppcArgv[3], pcExec);
  ppcArgv[4] = NULL;
    
  /* Flush buffers and fork. */
  fflush(NULL);
  iPid = fork();
  if(iPid == -1) {
    fprintf(stderr, "fork failure\n");
    return FAILURE;
  }
  
  /* Child process. */
  if (iPid == 0) {      
    /* Execute command. */
    execvp(ppcArgv[0], ppcArgv);
    iErrSv = errno;
    perror("server: compile: child");
    for (i = 0; i < 5; i++) free(ppcArgv[i]); free(ppcArgv);
    return FAILURE;
  }
   
  /* Parent process. */
  for (i = 0; i < 5; i++) free(ppcArgv[i]); free(ppcArgv);
  if ((iPid = wait(NULL)) == -1) {
    perror("server: compile: parent");
    return FAILURE;
  }
  return SUCCESS;  
}
