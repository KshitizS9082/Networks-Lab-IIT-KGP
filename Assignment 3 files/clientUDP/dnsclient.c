/*    THE CLIENT PROCESS */

#include <stdio.h>
#include <stdlib.h>	//for exit
#include <string.h>	//for strlen
#include <sys/types.h>	//for socket
#include <sys/socket.h>	//for socket
#include <netinet/in.h>	//for inet_aton
#include <arpa/inet.h>	//for htons
#include <unistd.h>		//for close

#define BUFFERSIZE 100

int main()
{
	//file descriptor
	int sockfd;	

	//structures for server and client addresses
	struct sockaddr_in serv_addr, cliaddr;

	//length for client address
	unsigned int len;

	int i, n;

	//we will use this buffer for communication
	char buf[BUFFERSIZE];

	//creating UDP socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

	//specifying server address details
	serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port = htons(8081);

	//connecting to the server socket
	if ((connect(sockfd, (struct sockaddr *) &serv_addr,
						sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}

	//emptying buffer
	for(i=0; i < BUFFERSIZE; i++) buf[i] = '\0';
	
	//reading domain name from the user
	scanf("%s",buf);

	//sending domain name to the server
	sendto(sockfd, (const char *)buf, strlen(buf), 0, 
			(const struct sockaddr *) &serv_addr, sizeof(serv_addr));
	
	while(1){

		//specifying length var
		len = sizeof(cliaddr);

		//receiving buf from server
	    n = recvfrom(sockfd, (char *)buf, BUFFERSIZE, 0, 
				( struct sockaddr *) &cliaddr, &len);  

	    buf[n] = '\0';

	    //break the loop if the received buffer is an empty string
	    if(buf[0]=='\0'){
	    	break;
	    }

	    //print the buffer
	    printf("%s\n",buf);
	}

	//close the socket
	close(sockfd);

	return 0;
}
