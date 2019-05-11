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
#include <errno.h>		//for errno

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

	//loop variables
	int i,q;

	//return val from udp recvfrom call
	int retVal;

	//variables for reading file into buffer
	int nread, k;

	//We will use this buffer for communication
	char buf[BUFFERSIZE];

	//a tempbuffer to read one character at a time
	char tempBuf[2];

	struct hostent *tempStructure;

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
	
	//making tcp socket nonblocking
	fcntl(sockfd1, F_SETFL, O_NONBLOCK);

	//can listen upto 5 connections on TCP sockfd1 concurrently
	listen(sockfd1, 5);

	while (1) {

		sleep(1);

		//getting client details
		clilen = sizeof(cli_addr);

		//first accept all the tcp connections in a while loop
		while(1){

			//resetting errno
			errno = 0;

			//accepting and getting new accepted socket descriptor
			newsockfd = accept(sockfd1, (struct sockaddr *) &cli_addr, &clilen);

			if(newsockfd>=0){

				if(errno!=EAGAIN && errno!=EWOULDBLOCK){			

					// if(newsockfd<0){
					// 	perror("Accept error");
					// 	exit(0);
					// }
					
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

						close(fileDescWord);
						exit(0);
					}
					//closing file descriptor
					close(newsockfd);
				
				}
			}
			else{
				break;
			}
		}

		//resetting errno
		errno = 0;

		//getting client details
		clilen = sizeof(cli_addr);
			
		//emptying buffer
		for(q = 0; q<BUFFERSIZE;q++){
			buf[q] = '\0';
		}

		//receive buffer from the client
		retVal =  recvfrom(sockfd2, (char *)buf, BUFFERSIZE-1, MSG_DONTWAIT, ( struct sockaddr *) &cli_addr, &clilen);

		if(retVal>=0){

			if(errno!=EAGAIN && errno!=EWOULDBLOCK){

				if(fork()==0){
					
					//searching for IP address for the received buffer

					tempStructure = gethostbyname(buf);

					if(tempStructure==NULL){
						//ip address not found

						sendto(sockfd2, "\0", 1, 0, 
							(const struct sockaddr *) &cli_addr, clilen);
						exit(0);
					}

					addrList = (struct in_addr **) tempStructure->h_addr_list;

					//copying result in the buffer
					for(i=0; addrList[i]!=NULL; i++){

						strcpy(buf, inet_ntoa(*addrList[i]));
						
						//sending the result back to the client
						sendto(sockfd2, (const char *)buf, strlen(buf)+1, 0, 
							(const struct sockaddr *) &cli_addr, clilen);
					
					}

					sendto(sockfd2, "\0", 1, 0, 
							(const struct sockaddr *) &cli_addr, clilen);

					exit(0);
				}
			}
		}

	}

	close(sockfd1);
	close(sockfd2);

	return 0;
}
			
