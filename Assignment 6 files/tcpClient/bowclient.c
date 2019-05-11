/*    THE CLIENT PROCESS */

#include <stdio.h>
#include <stdlib.h>	//for exit	
#include <sys/types.h>	//for socket
#include <sys/socket.h>	//for socket
#include <netinet/in.h>	//for inet_aton
#include <arpa/inet.h>	//for htons
#include <unistd.h>		//for close

#define BUFFERSIZE 100

int main()
{
	//socket descriptor
	int sockfd;

	//word counter variable
	int wordCount = 0;

	//structure for server address
	struct sockaddr_in serv_addr;

	int i;

	//we will use this buffer for communication
	char buf[BUFFERSIZE];

	//create a TCP socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

	//specifying server address details
	serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port = htons(8079);

	//connecting to server
	if ((connect(sockfd, (struct sockaddr *) &serv_addr,
						sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}

	while(1){

		//emptying buffer
		for(i=0; i < BUFFERSIZE; i++) buf[i] = '\0';
		
		//receiving buffer from server
		recv(sockfd, buf, BUFFERSIZE, 0);
		
		//if buffer is an empty string, break
		if(buf[0]=='\0'){
			break;
		}
		else{
			//else it is a word, increment word count
			wordCount++;
		}
	}
	
	//display the total number of words received
	printf("Number of words received: %d\n",wordCount);

	//close socket file descriptor
	close(sockfd);

	return 0;
}
