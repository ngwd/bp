#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h> 
#include <netdb.h> 
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main (int argc, const char* argv[]) {
  int argu = 1;
  if (argc > 1) 
	{
    argu = atoi(argv[1]);
  }
  
  struct sockaddr_in saddr;
  int fd, ret_val;
  struct hostent *local_host; /* need netdb.h for this */

  /* create a tcp socket */
  fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
  if (fd == -1) {
 	 fprintf(stderr, "socket failed [%s]\n", strerror(errno));
 	 return -1;
  }
  printf("socket(fd) %d created\n", fd);

  /* initialize the server address */
  saddr.sin_family = AF_INET;		 
  saddr.sin_port = htons(7000);	 
  local_host = gethostbyname("127.0.0.1");
  saddr.sin_addr = *((struct in_addr *)local_host->h_addr);

  /* connect to the server socket */
  ret_val = connect(fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
  if (ret_val == -1) {
 	 fprintf(stderr, "connection failed [%s]\n", strerror(errno));
 	 close(fd);
 	 return -1;
  }
  printf("connected\n");

  // printf("sleep before ending data\n");
  // sleep(5);

  /* send data */
  int even = (argu & 0x1) == 0;
  ret_val = send(fd, even ? "S" : "L", 2, 0);
  printf("Successfully sent data (len %d bytes) \n", ret_val);

  /* close the socket */
  close(fd);
  return 0;
}
