#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h> 
#include <netdb.h> 
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define DATA_BUFFER 1024 

int main (int argc, const char* argv[]) 
{
	int argu = 1;
	// for(int i = 0; i< argc; ++i) {printf("-- %s\n", argv[i]);}
	if (argc > 1) 
	{
		argu = atoi(argv[1]);
	}
	
	struct sockaddr_in saddr;
	int fd, ret_val;
	struct hostent *local_host; /* need netdb.h for this */

	/* create a tcp socket */
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
	if (fd == -1) 
	{
 	 fprintf(stderr, "socket failed [%s]\n", strerror(errno));
 	 return -1;
	}
	printf("socket(fd) %d created\n", fd);

	// initialize the server address */
	saddr.sin_family = AF_INET;		 
	saddr.sin_port = htons(7000);	 
	local_host = gethostbyname("127.0.0.1");
	saddr.sin_addr = *((struct in_addr *)local_host->h_addr);

	// connect to the server socket 
	ret_val = connect(fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
	if (ret_val == -1) {
 	 fprintf(stderr, "connection failed [%s]\n", strerror(errno));
 	 close(fd);
 	 return -1;
	}
	printf("connected\n");

	// printf("sleep before ending data\n");
	// sleep(5);

	// send data 
	char buf[DATA_BUFFER] = {0};
	int odd = argu & 0x1; 
  char *content = odd ? "S" : "L";
  int loop_times = argu % 1000;
	// while (1) {
  for(int i = 0; i < loop_times; ++i)
	{
		ret_val = send(fd, content, 2, 0);
		printf("%d sent data :%s (len %d bytes) \n", argu, content, ret_val);
		ret_val = recv(fd, buf, DATA_BUFFER, 0);
		printf("%d received data :%s (len %zu bytes) \n", argu, buf, strlen(buf));
	}	

	/* close the socket */
	close(fd);
	return 0;
}
