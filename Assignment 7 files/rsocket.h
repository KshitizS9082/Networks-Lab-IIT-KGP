#ifndef RSOCKET_H
#define RSOCKET_H

//Header files
#include <sys/types.h>		//for socket
#include <sys/socket.h>		//for socket
#include <netinet/in.h>		//for struct sockaddr_in
#include <sys/time.h>		//for struct timeval
#include <pthread.h>		//for pthread
#include <stdlib.h>			//for calloc
#include <string.h>			//for memcpy
#include <unistd.h>			//for select
#include <stdio.h>		

//Macros
#define MAXMSGLEN		101
#define MAXMSGS			100
#define SOCK_MRP		100
#define TIMEOUT_SEC 	2
#define TIMEOUT_USEC	0
#define DROP_PROB		0.05

//structure for a message
struct message{
	char msg[MAXMSGLEN];			//message, maximum 100 bytes
	size_t len;						//length of the message
	struct sockaddr_in socketaddr;	//sockaddr structure of the src/dest (whichever applicable)
};

//structure for a message node
struct msgNode{
	struct message m;				//the message
	struct msgNode *next;			
};

//structure for a node of unacknowledged-message table
struct unackMsgNode{
	struct message m;				//the message
	int id;							//message id
	struct timeval sendingTime;		//sending time as a timeval structure
	struct unackMsgNode *next;
};


//function prototypes
void setTimeout(struct timeval *tv);
void* threadX(void* param);
int r_socket(int domain, int type, int protocol);
int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t r_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t r_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
void HandleRetransmit(int sockfd);
void HandleReceive(int sockfd, struct sockaddr_in* src_addr, socklen_t addrlen, char *buf, int msgLen);
void HandleAppMsgRecv(int sockfd, struct sockaddr_in* src_addr, socklen_t addrlen, char *buf, int msgLen);
void HandleACKMsgRecv(char msgId);
int r_close(int fd);
int dropMessage(double p);

#endif