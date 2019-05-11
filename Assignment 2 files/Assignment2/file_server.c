/*    THE SERVER PROCESS */

#include <stdio.h>
#include <stdlib.h>		//for exit
#include <string.h>		//for strlen
#include <unistd.h>		//for write, close
#include <sys/types.h>	//for send, recv, open, socket, bind, listen, accept
#include <sys/stat.h>	//for open
#include <fcntl.h>		//for open
#include <sys/socket.h> //for send, recv, socket, bind, listen, accept
#include <netinet/in.h>	
#include <arpa/inet.h>	//for htons

#define BUFFERSIZE 100

int main()
{
	int	sockfd, newsockfd ; 
	int	clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	int i;
	char buf[BUFFERSIZE];		

	//creating socket	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port	= htons(8182);

	//binding the server with a name	
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

	//listening for connections on this socket, 5 is the max length of connections
	listen(sockfd, 5); 

	while(1){

		//accepting connection on this socket
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
						&clilen) ;
		
		if (newsockfd < 0) {
				perror("Accept error\n");
				exit(0);
			}

		//clearing buffer
		for(i=0; i < BUFFERSIZE; i++) buf[i] = '\0';
		
		//receiving filename from client
		recv(newsockfd, buf, BUFFERSIZE, 0);

		//opening file
		int fileDesc = open(buf, O_RDONLY);
		ssize_t nread;

		if(fileDesc >= 0){
			//if fileDesc is -1, then file does not exist

			while(1){
				//read from the file descriptor, chunk size 10 chosen
				nread = read(fileDesc, buf, 10);

				//if no bytes are read, then file reading is done
				if(nread==0){
					break;
				}

				//last char as null
				buf[nread] = '\0';

				//sending buffer to client
				send(newsockfd, buf, strlen(buf), 0);

				//clearing buffer
				for(i=0; i < BUFFERSIZE; i++) buf[i] = '\0';
			}
		}

		//closing file descriptor
		close(fileDesc);

		//closing socket descriptor
		close(newsockfd);
	}
	return 0;
}
			
