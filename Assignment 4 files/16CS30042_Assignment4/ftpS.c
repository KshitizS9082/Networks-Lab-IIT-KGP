#include <stdio.h>
#include <stdlib.h>		//for exit
#include <sys/types.h>	//for socket
#include <sys/socket.h>	//for socket
#include <arpa/inet.h>	//for htons
#include <unistd.h>		//for close
#include <string.h>		//for strtok
#include <sys/stat.h>	//for open
#include <fcntl.h>		//for open
#include <sys/wait.h>	//for wait


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

int parseCmd(char* cmdBuffer, char** cmdArgs, int* cmdNumber, int* portY, int* fileDesc){
	//call strtok to get cmdArgs
	int i = 0;
	int valReturned;

	//get the cmd name
	cmdArgs[i] = strtok(cmdBuffer," ");
	
	if(cmdArgs[i]==NULL){
		*cmdNumber = -1;
		return CMDNOTFOUND;
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
		*cmdNumber = 0;
		if(i!=2){
			return INVALIDARGS;
		}

		//get value of port Y
		valReturned = convStr2Int(cmdArgs[1]);
		
		//if port Y's value isn't proper, then intReturned would be -1
		if(valReturned<0){
			return ERRORCODE550;
		}

		*portY = valReturned;
	}
	else if(strcmp(cmdArgs[0],"cd")==0){
		*cmdNumber = 1;
		if(i!=2){
			return INVALIDARGS;
		}

		//change working directory
		valReturned = chdir(cmdArgs[1]);

		//error in changing working directory
		if(valReturned<0){
			return INVALIDARGS;
		}
	}
	else if(strcmp(cmdArgs[0],"get")==0){
		*cmdNumber = 2;
		if(i!=2){
			return INVALIDARGS;
		}

		//cmdArgs[1] will be the fileName
		//check if the file exists

		*fileDesc = open(cmdArgs[1],O_RDONLY);
		if(*fileDesc==-1){
			return ERRORCODE550;
		}

	}
	else if(strcmp(cmdArgs[0],"put")==0){
		*cmdNumber = 3;
		if(i!=2){
			return INVALIDARGS;
		}

				/*
		create a new file and write received data into it
		permission here is S_IRWXU, but can be S_IWUSR because 
		client will be just writing into this file. Permission is
		S_IRWXU so that it can be easily read while evaluation
		*/

		*fileDesc = open(cmdArgs[1], O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
		if(*fileDesc==-1){
			return ERRORCODE550;
		}
	}
	else if(strcmp(cmdArgs[0],"quit")==0){
		*cmdNumber = 4;
		if(i!=1){
			return INVALIDARGS;
		}
		return QUITRESPONSE;

	}
	else{
		*cmdNumber = -1;
		return CMDNOTFOUND;
	}

	return SUCCESS;
}

int main(){

	//process Sc is the parent process
	//Step 1: Create TCP socket S1

	//socket descriptor for S1, S2 and accepted socket
	int sockfdS1, sockfdS2, acceptedfd;

	//defining port X and port Y
	int port_X = 50000;
	int port_Y = 55000;

	//structure to store socket address
	struct sockaddr_in sock_addrS1, cli_addrC1, serv_addrC2;	

	//to store size of client socket address
	unsigned int clilen;

	//to get cmd number
	int cmdNumber;

	//loop variable
	int i;

	//status of child process
	int status;

	//fileDescriptor for get command
	int fileDesc;

	//create a buffer for command
	char cmdBuffer[CMDSIZE];
	char tempBuffer[BUFFERSIZE];
	char tempPastBuffer[BUFFERSIZE];

	//list of strings for command segments
	char **cmdArgs;

	//allocating space for cmdArgs
	cmdArgs = (char**)malloc((MAXNUMARGS)*sizeof(char*));
	for(i=0; i<MAXNUMARGS ;i++){
		cmdArgs[i] = (char*)malloc(CMDSIZE * sizeof(char));
	}

	//variable for response code
	int responseCode;

	//create TCP socket S1
	if ((sockfdS1 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket S1\n");
		exit(0);
	}

	//Step 2: Bind S1 to port X

	//specifying socket address S1
	sock_addrS1.sin_family = AF_INET;
	sock_addrS1.sin_addr.s_addr = INADDR_ANY;
	sock_addrS1.sin_port = htons(port_X);

	const int trueFlag = 1;
	if (setsockopt(sockfdS1, SOL_SOCKET, SO_REUSEADDR, &trueFlag, sizeof(int)) < 0)
		perror("Failure");

	if(bind(sockfdS1, (struct sockaddr *) &sock_addrS1, sizeof(sock_addrS1)) < 0) {
		printf("Unable to bind local address for TCP socket S1\n");
		exit(0);
	}

	//Step 3: Wait on S1

	//listen on S1
	listen(sockfdS1, 5);

	while(1){
		//storing size of client socket address in clilen variable
		clilen = sizeof(cli_addrC1);

		//accepting and getting new accepted socket descriptor, blocking call
		acceptedfd = accept(sockfdS1, (struct sockaddr *) &cli_addrC1, &clilen);

		int counter = 0;

		while(1){
		
			//Step 4: Receive command from C1
			recv(acceptedfd, cmdBuffer, CMDSIZE, 0);

			//Step 5: Parse the command and get the response code
			responseCode = parseCmd(cmdBuffer, cmdArgs, &cmdNumber, &port_Y, &fileDesc);

			if(counter==0 && cmdNumber!=0){
				//this is the first command and it is not port
				responseCode = PORTISNOTFIRSTCMD;
			}

			//Step 6: Run get or put command if cmdNumber is 2 or 3 and responseCode is SUCCESS
			if((cmdNumber==2||cmdNumber==3) && responseCode==SUCCESS){
				//this is a get or a put command
				//cmdArgs[1] is the filename
				//fileDesc contains the fileDescriptor of the file
			
				if(fork()==0){
					//this is process Sd

					//Step 1 : create a TCP socket S2
					if ((sockfdS2 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
						perror("Unable to create socket S2\n");
						exit(1);
					}

					//Step 2 : Connect to server C2 on port Y

					//specifying server address C2
					serv_addrC2.sin_family = AF_INET;
					inet_aton("127.0.0.1", &serv_addrC2.sin_addr);
					serv_addrC2.sin_port = htons(port_Y);

					//connecting with server
					if ((connect(sockfdS2, (struct sockaddr *) &serv_addrC2,
										sizeof(serv_addrC2))) < 0) {
						perror("Unable to connect to server C2\n");
						exit(1);
					}

					//if it is a get command, then read the file and send it to C2
					if(cmdNumber==2){
						
						//nread variable for number of bytes read
						ssize_t nread;
						char headerChar = 'N';
						unsigned short len;
						uint16_t length;
						int temp1,temp2, q;

						int TempCount = 0;

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
								send(sockfdS2, &headerChar, sizeof(headerChar), 0);
								send(sockfdS2, &length, sizeof(length), 0);
								send(sockfdS2, tempPastBuffer, temp2, 0);

								break;
									
							}

							if(TempCount == 0){
								
							}
							else{

								len = (unsigned short)(temp2);
								length = htons(len);

								//sending buffer to C2
								send(sockfdS2, &headerChar, sizeof(headerChar), 0);
								send(sockfdS2, &length, sizeof(length), 0);
								send(sockfdS2, tempPastBuffer, temp2, 0);

							}

							for(q = 0; q<temp1; q++){
								tempPastBuffer[q] = tempBuffer[q];
								temp2 = temp1;
							}

							TempCount++;
						}
					}
					else{

						char headerChar;
						uint16_t length;
						unsigned short len, temp;

						do{
							//receiving buffer from S2
							recv(sockfdS2, &headerChar, sizeof(headerChar), 0);
							recv(sockfdS2, &length, sizeof(length), 0);

							len = ntohs(length);
							
							while(len>0){
								temp = recv(sockfdS2,tempBuffer,len,0);
								len -= temp;
								write(fileDesc, tempBuffer, temp);
							}
						}while(headerChar!='L');

					}

					//close the file descriptor
					close(fileDesc);
					close(sockfdS2);

					exit(0);
				}
				else{
					wait(&status);
					if(status==0){
						responseCode = TRANSFERSUCCESSFUL;
					}
					else if(status==256){
						responseCode = ERRORCODE550;
					}
				}				

			}

			//send the response code to the client
			uint32_t rC = htonl(responseCode);

			send(acceptedfd, &rC, sizeof(rC), 0);

			if(responseCode == QUITRESPONSE 
				|| (responseCode == ERRORCODE550 && cmdNumber==0)
				|| responseCode == PORTISNOTFIRSTCMD){
				break;
			}

			counter++;
		}

		close(acceptedfd);
	}

	//free the allocated memory to cmdArgs
	free(cmdArgs);

	//close sockets
	close(sockfdS1);
	close(acceptedfd);

	return 0;
}


/*
1. This is a TCP server.
2. It will have two processes:
	2.1 Sc : the server's control process
	2.2 Sd : the server's data process

---Process Sc---
1) It will create a TCP socket S1.									DONE
2) Bind S1 to port X.												DONE
3) Wait on S1.														DONE
4) Receive command from C1 											DONE
5) Parse the command and get the response code 						DONE
6) If it is a get command, check if the file exists					DONE		


---Process Sd---
1) It will be forked by the process Sc when a data transfer has to be made.	DONE
2) It will create a TCP socket S2.											DONE
3) Using S2, it will connect to the client's data port  which is port Y.	DONE
4) Run get command if cmdNumber is 2 and responseCode is SUCCESS

Control Port 	: S1
Data Port		: S2

After each data transfer, S2 will be closed and will be created again for the next transfer. 
*/
