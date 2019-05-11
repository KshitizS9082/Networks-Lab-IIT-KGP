// A UDP Server implementation
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
  
#define MAXLINE 1024 
  
void read_words (FILE *f, char *buffer) {

    int i = 0;
    char c;

    while(i==0){
        while((c = fgetc(f))!=EOF){
            
            if(c==' ' || c=='\n' || c=='\t'){
                break;
            }
            else{
                buffer[i] = c;
                i++;
            }
        }
    }

    buffer[i] = '\0';
}


int main() { 
    int sockfd; 
    struct sockaddr_in servaddr, cliaddr; 
      
    // Create socket file descriptor 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockfd < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
      
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(8181); 
      
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    printf("\nServer Running....\n");

    while(1){
	    int n; 
	    socklen_t len;
	    char buffer[MAXLINE]; 
	 
	    //receiving from client and storing it in buffer

	    len = sizeof(cliaddr);
	    n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0, 
				( struct sockaddr *) &cliaddr, &len); 
	    buffer[n] = '\0'; 

	    printf("RECEIVED:%s\n", buffer);

	    //the buffer received must be the file name according to the protocol, opening this file
	    FILE *ifp;

	    //open the file with filename as buffer
	    ifp = fopen(buffer, "r");   

	    if(ifp==NULL){
	        //file not present

	        char *message = "NOTFOUND";

	        sendto(sockfd, (const char *)message, strlen(message), 0, 
	            (const struct sockaddr *) &cliaddr, sizeof(cliaddr));  
	        
	        printf("SENT:%s\n", message);

	        exit(1);
	    }

	    //reading the first line of the file

	    char line[MAXLINE];

	    fscanf(ifp,"%[^\n]", line);
	    fgetc(ifp); //for newline

	    //sending this line back to client

	    sendto(sockfd, (const char *)line, strlen(line), 0, 
	            (const struct sockaddr *) &cliaddr, sizeof(cliaddr));  
	        
	    printf("SENT:%s\n", line);
	    
	    //receiving message WORDi from client

	    char wordREAD[MAXLINE];

	    int i = 1;
	    while(1){

	        len = sizeof(cliaddr);
	        n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0, 
	                ( struct sockaddr *) &cliaddr, &len); 
	        buffer[n] = '\0';
	        printf("RECEIVED:%s\n", buffer);

	        read_words(ifp, wordREAD);

	        //sending ith word to client

	        sendto(sockfd, (const char *)wordREAD, strlen(wordREAD), 0, 
	                (const struct sockaddr *) &cliaddr, sizeof(cliaddr));  
	            
	        printf("SENT:%s\n", wordREAD);

	        if(strcmp(wordREAD,"END")==0){
	        	break;
	        }

	        i++;
	    }

	    fclose(ifp);
	}
    close(sockfd);

    return 0; 
} 