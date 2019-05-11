/* THE SERVER PROCESS */

#include <stdio.h>
#include <stdlib.h>		//for exit
#include <sys/types.h>	//for socket
#include <sys/socket.h> //for socket
#include <arpa/inet.h>	//for htons
#include <sys/types.h>	//for fork
#include <unistd.h>		//for fork, close
#include <netdb.h>		//for gethostbyname
#include <string.h>		//for strcpy
#include <sys/stat.h>	//for open
#include <fcntl.h>		//for open
#include <netinet/in.h>	//for inet_ntoa

#define BUFFERSIZE 100

int main()
{
	int	sockfd1;	//socket descriptor for TCP
	int sockfd2;	//socket descriptor for UDP
	int newsockfd;	//socket descriptor for the accepted socket
	
	unsigned int clilen;		//length for client address

	//structures for server and client addresses
	struct sockaddr_in cli_addr, serv_addr1, serv_addr2;	
	
	//to store gethostbyname returned list
	struct in_addr **addrList;

	//file descriptor for reading words.txt file
	int fileDescWord;

	//for select
	int nfds, r;

	//loop variable
	int i;

	//variables for reading file into buffer
	int nread, k;

	//We will use this buffer for communication
	char buf[BUFFERSIZE];

	//a tempbuffer to read one character at a time
	char tempBuf[2];

	//file descriptor set
	fd_set readfs;

	//creating TCP socket with sockfd1 file descriptor
	if ((sockfd1 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create TCP socket at server side\n");
		exit(0);
	}

	//creating UDP socket with sockfd2 file descriptor
	if ((sockfd2 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Cannot create UDP socket at server side\n");
		exit(0);
	}

	//specifying server address 1 details

	serv_addr1.sin_family = AF_INET;
	serv_addr1.sin_addr.s_addr = INADDR_ANY;
	serv_addr1.sin_port = htons(8080);

	//specifying server address 2 details

	serv_addr2.sin_family = AF_INET;
	serv_addr2.sin_addr.s_addr = INADDR_ANY;
	serv_addr2.sin_port = htons(8081);


	//binding TCP socket with the local address
	if (bind(sockfd1, (struct sockaddr *) &serv_addr1, sizeof(serv_addr1)) < 0) {
		printf("Unable to bind local address for TCP socket\n");
		exit(0);
	}


	//binding UDP socket with the local address
	if (bind(sockfd2, (struct sockaddr *) &serv_addr2, sizeof(serv_addr2)) < 0) {
		printf("Unable to bind local address for UDP socket\n");
		exit(0);
	}

	
	//can listen upto 5 connections on TCP sockfd1 concurrently
	listen(sockfd1, 5);

	while (1) {

		//zero all bits
		FD_ZERO(&readfs);

		//sets the bits to indicate which file descriptors to wait for reading
			
		FD_SET(sockfd1, &readfs);
		FD_SET(sockfd2, &readfs);

		//nfds = max(sockfd1, sockfd2) + 1

		if(sockfd1>sockfd2){
			nfds = sockfd1+1;
		}
		else{
			nfds = sockfd2+1;
		}

		//will block until atleast one of sockfd1 and sockfd2 is ready to read
		r = select(nfds, &readfs, NULL, NULL, NULL);

		//-1 denotes error
		if(r==-1){
			perror("select error");
			exit(0);
		}

		//true if TCP client has tried to connect with the server
		if(FD_ISSET(sockfd1, &readfs)){

			//getting client details
			clilen = sizeof(cli_addr);

			//accepting and getting new accepted socket descriptor
			newsockfd = accept(sockfd1, (struct sockaddr *) &cli_addr, &clilen);

			if(newsockfd<0){
				perror("Accept error");
				exit(0);
			}
			
			if(fork()==0){

				//old socket no longer required in the child process
				close(sockfd1);
				
				//emptying buffer
				for(i=0; i < BUFFERSIZE; i++) buf[i] = '\0';

				//opening file words.txt
				fileDescWord = open("word.txt", O_RDONLY);

				//resetting value of k to zero
				k = 0;
				while(1){
					//read a character from the file
					nread = read(fileDescWord, tempBuf, 1);
					
					//true if EOF is reached
					if(nread==0){
						//if something is left in the buffer
						if(k!=0){
							buf[k] = '\0';
							//send it to the client
							send(newsockfd, buf, BUFFERSIZE, 0);
							k = 0;
						}
						//then break
						break;
					}
					//each line has one word
					if(tempBuf[0]=='\n'){
						//got a word, terminating by '\0'
						buf[k] = '\0';
						send(newsockfd, buf, BUFFERSIZE, 0);
						//resetting k to zero
						k = 0;
					}
					else{
						//if everything is fine, store the character in the buffer
						if(k<BUFFERSIZE-2){
							buf[k] = tempBuf[0];
							k++;
						}
					}
				}

				//last empty string
				strcpy(buf, "");
				send(newsockfd, buf, BUFFERSIZE, 0);
				
				//closing file descriptor
				close(newsockfd);
				exit(0);
			}
			//closing file descriptor
			close(newsockfd);
		}

		if(FD_ISSET(sockfd2, &readfs)){

			//getting client details
			clilen = sizeof(cli_addr);
			if(fork()==0){
				//emptying buffer
				for(i=0; i < BUFFERSIZE; i++) buf[i] = '\0';
				
				//receive buffer from the client
				recvfrom(sockfd2, (char *)buf, BUFFERSIZE-1, 0, ( struct sockaddr *) &cli_addr, &clilen);
				
				//searching for IP address for the received buffer
				addrList = (struct in_addr **) gethostbyname(buf)->h_addr_list;
				
				//copying result in the buffer
				for(i=0; addrList[i]!=NULL; i++){

					strcpy(buf, inet_ntoa(*addrList[i]));
					
					//sending the result back to the client
					sendto(sockfd2, (const char *)buf, strlen(buf), 0, 
						(const struct sockaddr *) &cli_addr, clilen);
				
				}
				strcpy(buf,"");
				sendto(sockfd2, (const char *)buf, strlen(buf), 0, 
						(const struct sockaddr *) &cli_addr, clilen);

				exit(0);
			}
		}

	}

	return 0;
}
			
