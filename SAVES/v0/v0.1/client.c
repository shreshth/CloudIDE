#include "client.h"

int main(int argc, char **argv)
{
  /* variable declarations and initializations */
  int iSockFD = 0;
  struct sockaddr_in sServAddr;
  bzero(&sServAddr, sizeof(sServAddr));
  
  /* check usage */
  if (argc != 5) {
    fprintf(stderr, "usage: client <server> <port> <source> <exec>\n");
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
  sServAddr.sin_port = htons(atoi(argv[2]));
  inet_pton(AF_INET, argv[1], &sServAddr.sin_addr);
  
  /* connect */
  if (connect(iSockFD, (struct sockaddr *) &sServAddr, sizeof(sServAddr)) < 0) {
    fprintf(stderr, "connect failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  
  /* send file to server */
  if (Common_sendFile(iSockFD, argv[3]) == FAILURE) {
    exit(EXIT_FAILURE);
  }
  
  printf("WAITING"); fflush(NULL); // DEBUG
  
  /* receive file from server */
  if (Common_recvFile(iSockFD, argv[4]) == FAILURE) {
    exit(EXIT_FAILURE);
  }
  
  printf("ENDED"); fflush(NULL); // DEBUG
  
  exit(EXIT_SUCCESS);
}
