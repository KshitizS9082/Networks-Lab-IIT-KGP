#include "rsocket.h"

//initialize id to zero, since maximum 100 ids will be there, we can implement it by a char
char id = 0;

//number of transmissions
int numTransmissions = 0;

//thread id
pthread_t tid;
pthread_mutex_t mutex;
pthread_mutexattr_t mutex_attr;

//data structures
struct msgNode* 		recvBuffer;			//recvBuffer is a linked list's head
struct unackMsgNode* 	unackMsgTable;		//unackMsgTable is a linked list's head
int* 					recvMsgIdTable;		//stores all distinct ids received so far

void setTimeout(struct timeval *tv){
	tv->tv_sec = TIMEOUT_SEC;
	tv->tv_usec = TIMEOUT_USEC;
}

//runner function for thread X
void* threadX(void* param){

	int sockfd;

	sockfd = *((int*)(param));


	int nfds, r;
	fd_set readfds;
	struct timeval tv;
	setTimeout(&tv);

	while(1){

		FD_ZERO(&readfds);
		FD_SET(sockfd, &readfds);
		nfds = sockfd + 1;

		/*Waits on a select() call and comes out 
		either to receive message from the UDP socket or on a timeout T */

		r = select(nfds, &readfds, NULL, NULL, &tv);

		if(r==0){
			//timeout T has occurred
			//calls HandleRetransmit()

			HandleRetransmit(sockfd);

			//waits with fresh T
			setTimeout(&tv);
		}
		else{
			//some message is received

			char buf[MAXMSGLEN+2];
			bzero(buf, MAXMSGLEN+2);

			struct sockaddr_in src_addr;
			socklen_t length;
			ssize_t msgLen;

			msgLen = recvfrom(sockfd, (char *)buf, MAXMSGLEN+2, 0, (struct sockaddr *)&src_addr, &length);

			if(dropMessage(DROP_PROB)==0)
				HandleReceive(sockfd, &src_addr,length, buf,msgLen);
		}
	}
}

//API functions
int r_socket(int domain, int type, int protocol){

	srand(time(NULL));

	if(type==SOCK_MRP){

		//socket descriptor
		int sockfd;

		//Opens a UDP socket with the socket call
		sockfd = socket(domain, SOCK_DGRAM, protocol);

		if(sockfd < 0){
			return -1;
		}

		pthread_mutexattr_init(&mutex_attr);
		pthread_mutex_init(&mutex,&mutex_attr);

		int* param = (int*)malloc(sizeof(int));
		*param = sockfd;

		//Creates the thread X
		pthread_create(&tid,NULL,threadX,param);

		//Dynamically allocates space for all the tables and Initializes the tables
		recvBuffer 		= NULL;
		unackMsgTable 	= NULL;
		recvMsgIdTable 	= (int*)calloc(MAXMSGS,sizeof(int));

		return sockfd;
	}
	else{
		return -1;
	}
}

int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
	//Binds the UDP socket with the specified address-port using the bind call.
	return bind(sockfd, addr, addrlen);
}

ssize_t r_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen){

	//a new buffer
	char bufNew[MAXMSGLEN+2];
	bzero(bufNew, MAXMSGLEN+2);
	
	//'m' means application message here
	bufNew[0] = 'm';	 

	//Adds a unique id to the application message
	bufNew[1] = id;

	memcpy(&bufNew[2],buf,len);

	ssize_t returnVal;

	//lock
	pthread_mutex_lock(&mutex);

	//Sends the message with the id using the UDP socket in a single sendto() call.
	returnVal = sendto(sockfd, (const char *)bufNew, len+2, flags, dest_addr, addrlen);
	//printf("SENT_MSG\n");

	numTransmissions++;

	/*Updates unacknowledged-message table:
		Stores:
			Message
			Message ID
			Destination IP-port
			Sending time
	*/

	//dynamically allocating memory to a new node
	struct unackMsgNode* temp = (struct unackMsgNode*)malloc(sizeof(struct unackMsgNode));
	
	//storing original message
	memcpy(temp->m.msg, bufNew, len+2);
	temp->m.len = len+2;
	temp->m.socketaddr = *(struct sockaddr_in *)(dest_addr);

	//storing message id
	temp->id = id;

	//updating id
	id++;

	//updating sending time
	gettimeofday(&temp->sendingTime, NULL);

	//add this node at the front end of the linked list, i.e. at the head
	temp->next = unackMsgTable;
	unackMsgTable = temp;
	
	//unlock
	pthread_mutex_unlock(&mutex);


	if(returnVal<0){
		return -1;
	}

	//similar to sendto call
	return returnVal;
}

ssize_t r_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen){


	//lock
	pthread_mutex_lock(&mutex);

	while(recvBuffer==NULL){
		
		//unlock
		pthread_mutex_unlock(&mutex);

		//No message found
		//The user process is blocked
		//To block, sleep() call is used to wait for some time 
		//and then we see again if a message is received
		sleep(1);


		//relock
		pthread_mutex_lock(&mutex);

	}

	int i;

	//Message found
	struct msgNode* temp;
	temp = recvBuffer;

	//setting socket address structure and length
	src_addr = (struct sockaddr *)(&(temp->m.socketaddr));
	*addrlen = sizeof(temp->m.socketaddr);

	//check if the message is smaller or the requested length is smaller
	size_t msgSize;

	if((temp->m.len) <= len){
		msgSize = temp->m.len;
	}
	else{
		msgSize = len;
	}

	//Message is given to the user
	memcpy(buf,temp->m.msg,msgSize);

	if((temp->m.len) > len){
		//some message still remains which has not been given to the user

		//updating message
		for(i = len; i<temp->m.len; i++){
			temp->m.msg[i-len] = temp->m.msg[i]; 
		}

		//updating length field
		temp->m.len = temp->m.len - len + 1;

	}
	else{
		//temp's message has been fully read

		//updating the head pointer
		recvBuffer = recvBuffer->next;

		//freeing the memory location pointed by temp
		free(temp);
	}
	

	//unlock
	pthread_mutex_unlock(&mutex);

	// Function returns the number of bytes in the message returned.
	return msgSize;
		
}

void HandleRetransmit(int sockfd){
	// Handles any retransmission of app message.
	
	struct unackMsgNode* temp;
	temp = unackMsgTable;

	struct timeval ct;
	struct timeval result;

	//get current time
	gettimeofday(&ct, NULL);

	//lock
	pthread_mutex_lock(&mutex);

	// Scans the unacknowledged-message table
	while(temp!=NULL){

		//see if any of the messages' timeout period T is over

		result.tv_sec = ct.tv_sec - (temp->sendingTime).tv_sec;
		//result.tv_usec =  ct.tv_usec - (temp->sendingTime).tv_usec;

		if(result.tv_sec >= TIMEOUT_SEC){
			//timeout has occurred
			//Retransmit this message
			sendto(sockfd, (const char *)temp->m.msg,temp->m.len,0,(const struct sockaddr *)(&(temp->m.socketaddr)),sizeof(temp->m.socketaddr));
			//printf("SENT_MSG\n");

			numTransmissions++;

			// Update the sending time for the message
			(temp->sendingTime).tv_sec = ct.tv_sec;
			(temp->sendingTime).tv_usec = ct.tv_usec;
		}

		//update temp
		temp = temp->next;
	}	

	//unlock
	pthread_mutex_unlock(&mutex);
}

void HandleReceive(int sockfd, struct sockaddr_in* src_addr, socklen_t addrlen, char *buf, int msgLen){

	if(buf[0]=='m'){
		//App Message
		HandleAppMsgRecv(sockfd, src_addr, addrlen, buf, msgLen);
	}
	else{
		//Ack Message
		HandleACKMsgRecv(buf[1]);
	}
}

void HandleAppMsgRecv(int sockfd, struct sockaddr_in* src_addr, socklen_t addrlen, char *buf, int msgLen){
	
	pthread_mutex_lock(&mutex);

	// Check received-message-id table if it is:
	if(recvMsgIdTable[(int)buf[1]]==0){

		recvMsgIdTable[(int)buf[1]] = 1;

		//not a duplicate message
		struct msgNode *temp = (struct msgNode *)malloc(sizeof(struct msgNode)); 
		
		//updating message details
		memcpy(temp->m.msg,buf+2,msgLen-2);
		temp->m.len = msgLen-2;
		temp->m.socketaddr = *(struct sockaddr_in *)src_addr;

		//add this node at the front end of the linked list, i.e. at the head
		temp->next = recvBuffer;
		recvBuffer = temp;
	}

	// ACK is sent.
	char bufACK[3];
	bufACK[0] = 'a';		//a is for ACK
	bufACK[1] = buf[1];		//buf[1] is id of the corresponding message
	bufACK[2] = '\0';		//null terminating the message
	
	pthread_mutex_unlock(&mutex);

	sendto(sockfd, (const char *)bufACK, 3, 0, (const struct sockaddr *)src_addr, addrlen);
	//printf("SENT_ACK\n");

}

void HandleACKMsgRecv(char msgId){
	
	// Msg is checked in the unacknowledged-message table:
	
	struct unackMsgNode *temp;
	struct unackMsgNode *prev;

	temp = unackMsgTable;
	prev = NULL;

	//lock
	pthread_mutex_lock(&mutex);

	while(temp!=NULL){

		if(temp->id == msgId){
			//message found in the table
			//message removed from the table
			if(prev!=NULL){
				prev->next = temp->next;
			}
			else{
				unackMsgTable = temp->next;
			}
			free(temp);
			break;
		}

		prev = temp;
		temp = temp->next;
	}

	//unlock
	pthread_mutex_unlock(&mutex);
}	

int r_close(int fd){
	
	pthread_mutex_lock(&mutex);

	//Closes the socket (Not immediately, first check if all the pending messages have been received at the receiving end)
	while(unackMsgTable!=NULL){
		pthread_mutex_unlock(&mutex);
		sleep(1);
		pthread_mutex_lock(&mutex);
	}

	printf("Number of transmissions : %d\n", numTransmissions);
	//printf("%d, ",numTransmissions);
	pthread_mutex_unlock(&mutex);
	pthread_mutex_destroy(&mutex);

	//Kills all threads and frees all memory associated with the socket.	
	//freeing recvBuffer
	while(recvBuffer!=NULL){
		struct msgNode* temp = recvBuffer;
		recvBuffer = recvBuffer->next;
		free(temp);
	}
	free(recvMsgIdTable);

	pthread_cancel(tid);

	return close(fd);
}

int dropMessage(double p){
	// p is a probability
	double num;
	// num = Generate a random number between 0 and 1
	num = (double)rand()/(double)RAND_MAX;

	if(num<p)
		return 1;
	else
	 	return 0;
}
