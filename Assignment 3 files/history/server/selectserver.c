/* THE SERVER PROCESS */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>


int main()
{
	//Socket descriptors
	int sockfd1, sockfd2;
	int	newsockfd;

	int	clilen;
	struct sockaddr_in	cli_addr, serv_addr;
	struct in_addr **addrList;

	int i, k;
	char buf[100];		/* We will use this buffer for communication */
	char word[100];

	//set of socket descriptors
	fd_set readfds;
	

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create socket\n");
		exit(0);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(8181);

	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		printf("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5);

	while (1) {

		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
					&clilen) ;

		if (newsockfd < 0) {
			printf("Accept error\n");
			exit(0);
		}

		if (fork() == 0) {

			close(sockfd);
			
			//opening file
			int fileDesc = open("word.txt", O_RDONLY);
			ssize_t nread;

			if(fileDesc>=0){

				while(1){
					nread = read(fileDesc, buf, 10);
				
					if(nread==0){
						break;
					}

					buf[nread] = '\0';

					for(i=0, k=0; buf[i]!='\0'; i++){
						if(buf[i]!='\n'){
							word[k++] = buf[i];
						}
						else{
							word[k++]='\n';
							word[k] = '\0';
							send(newsockfd, word, strlen(word), 0);
							printf("SENT: %s\n",word);
							k = 0;
						}
					}
				}
				strcpy(word,"");
				send(newsockfd, word, strlen(word), 0);
			}

			close(newsockfd);
			exit(0);
		}

		close(newsockfd);
	}

	close(sockfd);

	return 0;
}
			
