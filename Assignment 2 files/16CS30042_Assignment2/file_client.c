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

int isDelimiter(char c){
	if(c==','||c==';'||c==':'||c=='.'||c==' '||c=='\t'||c=='\n'){
		return 1;
	}
	else{
		return 0;
	}
}

int countWords(char* buf){

	int i = 0;
	int wordCount = 0, flag = 0;

	for(;buf[i]!='\0';i++){

		while(isDelimiter(buf[i])==0){
			/*
			flag =1 denotes word has started and flag = 0 denotes word is
			complete. Count as a word when it is complete.
			*/
			flag = 1;
			i++;
			if(buf[i]=='\0'){
				//count as a word if end is reached
				flag = 0;
				wordCount++;
				break;
			}
		}
		if(flag==1){
			wordCount++;
			flag = 0;
		}
		if(buf[i]=='\0'){
			break;
		}
	
	}
	return wordCount;
}

int main()
{
	int	sockfd;
	struct sockaddr_in	serv_addr;

	int i;
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
	send(sockfd, fileName, strlen(fileName), 0);

	//clearing buffer
	for(i=0; i < BUFFERSIZE; i++) buf[i] = '\0';
	
	//receiving buffer from server
	int x = recv(sockfd, buf, BUFFERSIZE, 0);

	if(x==0){
		/* x = 0 denotes either 0 bytes or the stream socket peer has
		performed an orderly shutdown */

		printf("File Not Found\n");
		close(sockfd);
		exit(0);
	}
	else{

		/*
		create a new file and write received data into it
		permission here is S_IRWXU, but can be S_IWUSR because 
		client will be just writing into this file. Permission is
		S_IRWXU so that it can be easily read while evaluation
		*/
		int fileDesc = open(fileName, O_WRONLY|O_CREAT, S_IRWXU);

		//counter variables for bytes and words
		int byteCount = 0, wordCount = 0;

		/*remembering the last character, initializing it as a delimiter
		to ensure a following condition will be false for the first time
		*/
		char lastChar=',';

		while(x!=0){
			//write returns number of bytes written
			byteCount += (int)(write(fileDesc, buf, strlen(buf)));

			//function countWords returns number of words, defined above
			wordCount += countWords(buf);

			if(isDelimiter(lastChar)==0 && isDelimiter(buf[0])==0){
				/*
				if the last letter wasn't a delimiter and now the first 
				character is also not a delimiter, then we have counted 
				a single word twice, first in the previous buffer and second
				in the current buffer, so subtract 1 from wordCount
				*/
				wordCount--;
			}

			lastChar = buf[strlen(buf)-1];

			//clearing buffer
			for(i=0; i < BUFFERSIZE; i++) buf[i] = '\0';
			
			//receiving buffer from server
			x = recv(sockfd, buf, BUFFERSIZE, 0);
		}	

		//printing file info, bytes and words
		printf("The file transfer is successful. Size of the file = %d bytes, no. of words = %d\n",byteCount, wordCount);

		//closing file descriptor
		close(fileDesc);
	}
	
	//closing socket descriptor
	close(sockfd);

	return 0;
}