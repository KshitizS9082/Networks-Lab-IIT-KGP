#include <stdio.h>
#include <stdlib.h>		//for exit
#include <sys/types.h>	//for socket
#include <sys/socket.h>	//for socket
#include <netinet/in.h>	//for inet_aton
#include <arpa/inet.h>	//for inet_aton
#include <unistd.h>		//for close
#include <string.h>		//for strlen
#include <fcntl.h>		//for pipe
#include <sys/wait.h>	//for wait
#include <errno.h>

#define BUFFERSIZE 100
#define BLOCKSIZE 50
#define CMDSIZE 80
#define MAXNUMARGS 3

#define SUCCESS 200
#define TRANSFERSUCCESSFUL 250
#define QUITRESPONSE 421
#define INVALIDARGS 501
#define CMDNOTFOUND 502
#define PORTISNOTFIRSTCMD 503
#define ERRORCODE550 550

void extractFileName(char *str, char *buf){

	int len = strlen(str);
	int i, pos = -10;

	for(i=len-1;i>=0;i--){
		if(str[i]=='/'){
			pos = i;
			break;
		}
	}
	if(pos!=-10){
		//copy characters from pos+1 to end of str into str

		for(i=pos+1;i<len;i++){
			buf[i-pos-1] = str[i];
		}
		buf[i-pos-1] = str[i];
	
	}
	else{
		strcpy(buf, str);
	}
}


int convStr2Int(char* str){
	int ans = 0;
	int i;

	for(i=0;str[i]!='\0';i++){
		if(str[i]>='0'&&str[i]<='9'){
			ans = ans*10 + (int)(str[i]-'0');
			if(ans>65535){
				return -1;
			}
		}
		else{
			return -1;
		}
	}

	if(ans<1024){
		return -1;
	}
	return ans;
}

int getTypeOfCommand(char* cmdBuffer, char** cmdArgs){
	int i = 0;

	//get the cmd name
	cmdArgs[i] = strtok(cmdBuffer," ");
	
	if(cmdArgs[i]==NULL){
		return -1;
	}

	//get all arguments
	while(cmdArgs[i]!=NULL){
		i++;
		if(i==MAXNUMARGS){
			//NULL is not at argument number 2
			break;
		}
		cmdArgs[i] = strtok(NULL," ");
	}

	if(strcmp(cmdArgs[0],"port")==0){
		return 0;
	}
	else if(strcmp(cmdArgs[0],"cd")==0){
		return 1;
	}
	else if(strcmp(cmdArgs[0],"get")==0){
		if(i!=2){
			return -1;
		}
		return 2;
	}
	else if(strcmp(cmdArgs[0],"put")==0){
		if(i!=2){
			return -1;
		}
		return 3;
	}
	else if(strcmp(cmdArgs[0],"quit")==0){
		return 4;
	}
	else{
		return -1;
	}

}

void displayMessage(int responseCode, int cmdNumber, int counter){
	
	printf("RESPONSE CODE : %d\n",responseCode);

	if(counter==0 && responseCode==PORTISNOTFIRSTCMD){
		printf("First command should be port! Exiting.\n");
	}
	else if(responseCode==INVALIDARGS){
		printf("Invalid Arguments\n");
	}
	else{

		if(cmdNumber==0){
			if(responseCode==SUCCESS){
				printf("Port Y successfully set\n");
			}
			else if(responseCode==ERRORCODE550){
				printf("Port Y's value should be between 1024 and 65535\n");
			}
		}
		else if(cmdNumber==1){
			if(responseCode==SUCCESS){
				printf("Directory change successful\n");
			}
		}
		else if(cmdNumber==2){
			if(responseCode==TRANSFERSUCCESSFUL){
				printf("File received successfully\n");
			}
			else if(responseCode==ERRORCODE550){
				printf("Error in file transfer\n");
			}
		}
		else if(cmdNumber==3){
			if(responseCode==TRANSFERSUCCESSFUL){
				printf("File sent successfully\n");
			}
			else if(responseCode==ERRORCODE550){
				printf("Error in file transfer\n");
			}
		}
		else if(cmdNumber==4){
			if(responseCode==QUITRESPONSE){
				printf("Closing connection and exiting\n");
			}
		}
		else{
			if(responseCode==CMDNOTFOUND){
				printf("Invalid Command\n");
			}
		}
	}
}


int main(){

	//process Cc is the parent process
	//Step 1 : create TCP socket C1

	//socket descriptor for C1
	int sockfdC1;

	//socket descriptor for C2
	int sockfdC2;

	//socket descriptor for accepted connection
	int acceptedfd;

	//defining port X
	int port_X = 50000;

	//defining port Y
	int port_Y = 55000;

	//loop var
	int i;

	//var for cmd number
	int cmdNumber;

	//variable to store clilen
	unsigned int clilen;

	//structure to store server address for socket S1
	struct sockaddr_in serv_addrS1;

	//structure to store socket address for socket C2 and client S2
	struct sockaddr_in sock_addrC2, cli_addrS2;

	//create a buffer for command
	char cmdBuffer[CMDSIZE];

	//buffers
	char buf1[BUFFERSIZE];
	char buf2[BUFFERSIZE];
	char buf3[BUFFERSIZE];
	char tempBuffer[BUFFERSIZE];
	char tempPastBuffer[BUFFERSIZE];

	strcpy(buf2,"");
	strcpy(buf3,"");

	//response code
	int responseCode;

	//create TCP socket C1
	if ((sockfdC1 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket C1\n");
		exit(0);
	}

	//Step 2 : C1 connects to port X of the server

	//specifying server socket address S1
	serv_addrS1.sin_family = AF_INET;
	inet_aton("127.0.0.1", &serv_addrS1.sin_addr);
	serv_addrS1.sin_port = htons(port_X);

	//connecting with server
	if ((connect(sockfdC1, (struct sockaddr *) &serv_addrS1,
						sizeof(serv_addrS1))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}

	//list of strings for command segments
	char **cmdArgs;

	pid_t p;

	int fileDesc, counter = 0;


	//allocating space for cmdArgs
	cmdArgs = (char**)malloc((MAXNUMARGS)*sizeof(char*));
	for(i=0; i<MAXNUMARGS ;i++){
		cmdArgs[i] = (char*)malloc(CMDSIZE * sizeof(char));
	}

	while(1){

		//Step 3 : Give a prompt ">" and wait for user commands.

		//print prompt in green colour
		printf("\033[1;32m> \033[0m");

		//wait for user command
		fgets(cmdBuffer, CMDSIZE, stdin);

		//removing newline from the command which appears is the last character
		cmdBuffer[strlen(cmdBuffer)-1] = '\0';

		//resetting buf1
		for(i=0; i<BUFFERSIZE; i++)	buf1[i] = '\0';
		
		//storing a copy of cmdBuffer
		strcpy(buf1, cmdBuffer);

		//getting type of command to check if it's a get or put command
		cmdNumber = getTypeOfCommand(cmdBuffer, cmdArgs);

		//retrieving copy of cmdBuffer back from buf1
		strcpy(cmdBuffer, buf1);

		if((cmdNumber==2 || cmdNumber==3) && (counter!=0)){
			//this is a get or a put command


			//cmdArgs[1] contains the filename (can be with path, will extract it)
			extractFileName(cmdArgs[1], buf2);
			//now buf2 contains filename

			if(cmdNumber == 3){
				fileDesc = open(buf2, O_RDONLY);
				if(fileDesc==-1){
					printf("RESPONSE CODE : %d\n",ERRORCODE550);
					
					printf("The file you wish to send doesn't exist in the local directory.\n");
					continue;
				}
			}

			//buf2 contains filename


			if((p=fork())==0){
				//this is process Cd

				//Step 1 : create a TCP socket C2
				if ((sockfdC2 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
					perror("Unable to create socket C2\n");
					exit(1);
				}

				//Step 2 : Bind C2 to port Y

				//specifying socket address C2
				sock_addrC2.sin_family = AF_INET;
				sock_addrC2.sin_addr.s_addr = INADDR_ANY;
				sock_addrC2.sin_port = htons(port_Y);


				const int trueFlag = 1;
				if (setsockopt(sockfdC2, SOL_SOCKET, SO_REUSEADDR, &trueFlag, sizeof(int)) < 0)
					perror("Failure");

				if(bind(sockfdC2, (struct sockaddr *) &sock_addrC2, sizeof(sock_addrC2)) < 0) {
					printf("Error code: %d\n", errno);
					printf("Unable to bind local address for TCP socket C2\n");
					exit(1);
				}

				//Step 3: Wait on C2

				//listen on C2
				listen(sockfdC2, 5);

				//storing size of client socket address in clilen variable
				clilen = sizeof(cli_addrS2);

				//accepting and getting new accepted socket descriptor
				acceptedfd = accept(sockfdC2, (struct sockaddr *) &cli_addrS2, &clilen);

				if(cmdNumber == 2){

					char headerChar;
					uint16_t length;
					unsigned short len, temp;

					fileDesc = open(buf2, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);

					do{
						//receiving buffer from S2
						recv(acceptedfd, &headerChar, sizeof(headerChar), 0);
						recv(acceptedfd, &length, sizeof(length), 0);

						len = ntohs(length);
						
						while(len>0){
							temp = recv(acceptedfd,buf1,len,0);
							len -= temp;
							write(fileDesc, buf1, temp);
						}
					}while(headerChar!='L');

					recv(acceptedfd, buf1, len, 0);

				}
				else{

					//nread variable for number of bytes read
					ssize_t nread;
					char headerChar = 'N';
					unsigned short len;
					uint16_t length;

					//clearing buffer
					for(i=0; i < BUFFERSIZE; i++) tempBuffer[i] = '\0';
					for(i=0; i < BUFFERSIZE; i++) tempPastBuffer[i] = '\0';

					int TempCount = 0;
					int temp1, temp2=0, q;

					while(1){
						//read from the file descriptor
						nread = read(fileDesc, tempBuffer, BLOCKSIZE);
						temp1 = nread;

						//if no bytes are read, then file reading is done
						if(nread==0){

							len = (unsigned short)(temp2);
							length = htons(len);

							headerChar = 'L';

							//sending buffer to C2
							send(acceptedfd, &headerChar, sizeof(headerChar), 0);
							send(acceptedfd, &length, sizeof(length), 0);
							send(acceptedfd, tempPastBuffer, temp2, 0);

							//break;
							while(1);
						}

						if(TempCount == 0){
							
						}
						else{

							len = (unsigned short)(temp2);
							length = htons(len);

							//sending buffer to C2
							send(acceptedfd, &headerChar, sizeof(headerChar), 0);
							send(acceptedfd, &length, sizeof(length), 0);
							send(acceptedfd, tempPastBuffer, temp2, 0);	
						}
						for(q = 0; q<temp1; q++){
							tempPastBuffer[q] = tempBuffer[q];
						}
						temp2 = temp1;

						TempCount++;
					}
				}

				//closing file descriptor
				close(fileDesc);

				//close accepted socket descriptor
				close(acceptedfd);

				//close socket descriptor for C2
				close(sockfdC2);
				exit(0);
			}
		}

		//Step 4 : Send the command to the server
		send(sockfdC1, cmdBuffer, CMDSIZE, 0);

		uint32_t rC;

		//Step 5: Receive the response code from the server
		recv(sockfdC1, &rC, sizeof(rC), 0);

		responseCode = ntohl(rC);

		//if response Code is success and cmdNumber is 0
		//then change value of port Y
		if(responseCode==SUCCESS && cmdNumber==0){
			port_Y = convStr2Int(cmdArgs[1]);
		}

		//client's Cd process will get over only after Sd's process has ended

		if(counter!=0){
			if((cmdNumber==2 && responseCode!=TRANSFERSUCCESSFUL) || cmdNumber==3){
				kill(p, SIGTERM);
			}
		}


		displayMessage(responseCode, cmdNumber, counter);

		//Step 7: Close connection and exit if error response code is obtained
		if(responseCode==QUITRESPONSE 
			|| responseCode==PORTISNOTFIRSTCMD
			|| (responseCode==ERRORCODE550 && cmdNumber==0)){
			break;
		}

		wait(NULL);
		counter++;
	}

	close(sockfdC1);	

	return 0;
}

/*
1. It will have two processes:
	1.1 Cc : the client's control process
	1.2 Cd : the client's data process

---Process Cc---
1. It will create a TCP socket C1.									DONE
2. C1 will connect to port X of the server.							DONE
3. Give a prompt ">" and wait for user commands.					DONE
4. User typed command is sent to the server.						DONE
5. Receive the response code from the server.						DONE
6. Print the response code alongwith meaningful message.
7. Close connection and exit if error response code is obtained.	DONE

---Process Cd---
1. It will create a TCP socket C2.									DONE
2. Bind C2 to port Y.												DONE
3. Wait on C2.														DONE
4. Accept on C2.													DONE
5. Receive file if it is a get command

Control Port	: C1
Data Port	: C2
*/
