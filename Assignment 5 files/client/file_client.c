/*    THE CLIENT PROCESS */

#include <stdio.h>		
#include <stdlib.h>		//for exit
#include <string.h>		//for strlen
#include <unistd.h>		//for write, close
#include <sys/types.h>	//for connect, send, recv, open, socket
#include <sys/stat.h>	//for open
#include <fcntl.h>		//for open
#include <sys/socket.h> //for connect, send, recv, socket
#include <netinet/in.h>	//for inet_aton
#include <arpa/inet.h>	//for inet_aton, htons

#define BUFFERSIZE 100


int main()
{
	int	sockfd;
	struct sockaddr_in	serv_addr;

	int i, B = 20;				//B is the block size
	char buf[BUFFERSIZE];

	//creating socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

	/* 
	   In this program, we assume that the server is running on the
	   same machine as the client. 127.0.0.1 is a special address
	   for "localhost"
	*/
	   
	serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(8182);

	//connecting with server
	if ((connect(sockfd, (struct sockaddr *) &serv_addr,
						sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}

	//variable to hold file name
	char fileName[BUFFERSIZE];

	//getting fileName from user 
	scanf("%s",fileName);

	//sending fileName to the server
	send(sockfd, fileName, strlen(fileName)+1, 0);

	//clearing buffer
	for(i=0; i < BUFFERSIZE; i++) buf[i] = '\0';
	
	char charMsg;

	recv(sockfd, &charMsg, sizeof(charMsg), 0);

	int FSIZE, wholeBlocks, lastBlockSize, fileDesc, recvRet;

	if(charMsg=='E'){

		//received E, so file not found

		printf("File Not Found\n");
	}
	else if(charMsg=='L'){

		//receive file size from the server
		
		recv(sockfd, &FSIZE, sizeof(FSIZE), MSG_WAITALL);
		
		//calculating whole blocks and block size of the last block

		wholeBlocks = (FSIZE / B);
		lastBlockSize = (FSIZE % B);


		//opening with S_IRWXU so that file can be opened without changing
		//permissions while evaluation, O_CREAT for creating new file,
		//O_TRUNC for truncating any previous file with same name
		fileDesc = open(fileName, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
		

		for(i=0; i<wholeBlocks; i++){			
			recvRet = recv(sockfd, buf, B, MSG_WAITALL);
			//i+1 blocks have been received each of size B

			//write the received buffer into the file
			write(fileDesc, buf, B);
		}

		//recv command to get last block
		recvRet = recv(sockfd, buf, lastBlockSize, MSG_WAITALL);
		write(fileDesc, buf, lastBlockSize);

		if(recvRet == 0){
			printf("Server closed down unexpectedly!\n");
			remove(fileName);
		}
		else{
			printf("The file transfer is successful. Total number of blocks received = %d, Last block size = %d\n", wholeBlocks+1, lastBlockSize);
		}
	}
	else{
		perror("Unknown character received");
	}

	//close file descriptor
	close(fileDesc);

	//closing socket
	close(sockfd);

	return 0;
}