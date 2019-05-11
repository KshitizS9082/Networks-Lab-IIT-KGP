// A UDP Client Implementation
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
  
#define MAXLINE 1024 

int main() { 
    int sockfd; 
    struct sockaddr_in servaddr; 
  
    // Creating socket file descriptor 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockfd < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(8181); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
      
    //sending file name to server

    int n;
    socklen_t len; 
    char *file_name = "animals.txt"; 
      
    sendto(sockfd, (const char *)file_name, strlen(file_name), 0, 
			(const struct sockaddr *) &servaddr, sizeof(servaddr));  
           
    char buffer[MAXLINE];

    n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0,
    	(struct sockaddr *) &servaddr, &len);
    buffer[n] = '\0';

    printf("RECEIVED:%s\n", buffer);

    if(strcmp(buffer,"NOTFOUND")==0){
    	printf("File Not Found\n");
    	exit(1);
    }

    FILE *ofp;
    char *outFileName = "local_animals.txt";

    ofp = fopen(outFileName,"w");

    //sending message WORDi to the server

    int i = 1;
    while(1){

	    char message[MAXLINE] = "WORD";
	    char digitSTR[MAXLINE] = "";
	    sprintf(digitSTR, "%d", i);
	    strcat(message, digitSTR);
	    sendto(sockfd, (const char *)message, strlen(message), 0, 
				(const struct sockaddr *) &servaddr, sizeof(servaddr));  

	    printf("SENT:%s\n", message);

	    //receiving 1st word from the server
	    n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0,
	    	(struct sockaddr *) &servaddr, &len);
	    buffer[n] = '\0';

	    printf("RECEIVED:%s\n", buffer);

        if(strcmp(buffer,"END")==0){
            break;
        }
        else{
            fprintf(ofp, "%s\n", buffer);
        }
        i++;
	}	

    fclose(ofp);
    close(sockfd); 
    return 0; 
} 