#include "common.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
  char buf[MAX_BUFF];
  //FILE *fd = fopen(argv[1], "rb");
  int fd = open(argv[1], O_RDWR);
  int n;

  /*
  while(n = fread(buf, 1, MAX_BUFF, fd)) {
    printf("\n*******************************\n");
    printf("%d sent: %s", strlen(buf), buf);
  }
    
  if (feof(fd)) printf("\nEOF");
  if (ferror(fd)) printf("\nERROR");
  */

  while (n = read(fd, buf, MAX_BUFF)) {
    printf("\n*******************************\n");
    printf("%d sent: %s", strlen(buf), buf);
  }


  return SUCCESS;
}
