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

int getFileSize(int fd, int *fileSize){
	struct stat statbuf;
	int returnVal;

	//call fstat command
	returnVal = fstat(fd, &statbuf);
	
	//extract filesize from the statbuf structure
	*fileSize = (int)(statbuf.st_size);

	return returnVal;
}

int main()
{
	//socket descriptors
	int	sockfd, newsockfd ; 
	unsigned int clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	//Block Size
	int B = 20;

	int i,x,k;
	char rdchar[5];

	char buf[BUFFERSIZE], fileBuf[BUFFERSIZE];		

	//creating socket	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

	//specifying serv address details
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port	= htons(8182);

	//in order to reuse socket immediately
	const int trueFlag = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &trueFlag, sizeof(int)) < 0)
		perror("Failure");

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
		
		//k is to point current empty fileBuf loc
		k = 0;
		

		//receiving buffer till the null character
		do{
			//clearing buffer
			for(i=0; i < BUFFERSIZE; i++) buf[i] = '\0';
	
			x = recv(newsockfd, buf, BUFFERSIZE, 0);

			//copy buf to filebuf
			for(i=0; k<BUFFERSIZE&&i<x ;k++,i++){
				fileBuf[k] =  buf[i];
			}

		}while(buf[x-1]!='\0' && k<BUFFERSIZE);

		//fileBuf contains the filename

		//opening file
		int fileDesc = open(fileBuf, O_RDONLY);

		char charMsg = 'L';
		int FSIZE;

		if(fileDesc<0){

			//file not found

			charMsg = 'E';
			//if fileDesc is -1, then file does not exist
			send(newsockfd, &charMsg, sizeof(charMsg), 0);
		}
		else{

			//file is present

			//send message 'L'
			send(newsockfd, &charMsg, sizeof(charMsg), 0);
			
			if(getFileSize(fileDesc, &FSIZE)!= 0){
				perror("Error Reading File Size");
				exit(0);
			}

			//sending file size to client
			send(newsockfd, &FSIZE, sizeof(FSIZE), 0);

			//calculating number of wholeblocks and size of the last block
			int wholeBlocks = (FSIZE / B);
			int lastBlockSize = (FSIZE % B);

			while(wholeBlocks--){

				//read B bytes from the file into the buffer buf
				x = read(fileDesc, buf, B);


				// x not equal to B, rarely happens
				if(x!=B){

					if(x>B){
						perror("Read more bytes! unknown error!");
						exit(0);
					}
					else{
						//read remaining bytes char by char
						while(x!=B){
							read(fileDesc, rdchar, 1);
							x++;
							buf[x-1] = rdchar[0];
						}
					}
				}

				//send the read buf of size B to the client
				send(newsockfd, buf, B, 0);
			}

			//read the last block
			x = read(fileDesc, buf, lastBlockSize);

			// x not equal to lastBlockSize, rarely happens

			if(x!=lastBlockSize){
				if(x>lastBlockSize){
					perror("Read more bytes! unknown error!");
					exit(0);
				}
				else{
					//read remaining bytes char by char
					while(x!=lastBlockSize){
						read(fileDesc, rdchar, 1);
						x++;
						buf[x-1] = rdchar[0];
					}
				}
			}


			//send the read buf of size B to the client
			send(newsockfd, buf, lastBlockSize, 0);

		}

		//closing file descriptor
		close(fileDesc);

		//closing socket descriptor
		close(newsockfd);
	}

	//close socket
	close(sockfd);

	return 0;
}
			
