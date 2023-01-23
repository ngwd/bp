#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h> 
#include <netdb.h> 
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#define DATA_BUFFER 1024 

int main (int argc, const char* argv[]) 
{
	int argu = 1;
	// for(int i = 0; i< argc; ++i) {printf("-- %s\n", argv[i]);}
	if (argc > 1) { argu = atoi(argv[1]); }
	
	struct sockaddr_in saddr;
	int fd, ret_val;
	struct hostent *local_host; // need netdb.h 

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
	if (fd == -1) 
	{
		fprintf(stderr, "socket failed [%s]\n", strerror(errno)); 
		return -1;
	}
	printf("socket(fd) %d created\n", fd);

	// initialize the server address 
	saddr.sin_family = AF_INET;		 
	saddr.sin_port = htons(7000);	 
	local_host = gethostbyname("127.0.0.1");
	saddr.sin_addr = *((struct in_addr *)local_host->h_addr);

	// connect to the server socket 
	ret_val = connect(fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
	if (ret_val == -1) 
	{
		fprintf(stderr, "connection failed [%s]\n", strerror(errno));
		close(fd);
		return -1;
	}
	printf("connected\n");

	int odd = argu & 0x1; 
	// int loop_times = argu % 1000;
	// while (1) {
  // srand(time(NULL));
	// for(int i = 0; i < 10; ++i)
	{
		// argu = rand() % 100 + 1;
		char buf[DATA_BUFFER] = {0};
		sprintf(buf, "%d", argu);
		ret_val = send(fd, buf, strlen(buf), 0);
		printf("%d sent data :%s (len %d bytes) \n", argu, buf, ret_val);

		char buf1[DATA_BUFFER] = {0}; 
		ret_val = 0;
		ret_val = recv(fd, buf1, DATA_BUFFER, 0);
		printf("%d received data :%s (len %zu bytes) \n", argu, buf1, strlen(buf1));
	}	
	close(fd);
	printf("%d closed \n", argu);
	return 0;
}
