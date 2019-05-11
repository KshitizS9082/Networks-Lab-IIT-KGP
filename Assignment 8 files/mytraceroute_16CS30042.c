#include <stdio.h>				//for printf, perror
#include <stdlib.h>				//for exit
#include <sys/types.h>			//for socket, select     
#include <sys/socket.h>			//for socket, inet_ntoa
#include <netinet/in.h>			//for IPPROTO_UDP, inet_ntoa
#include <linux/udp.h>			//for IPPROTO_UDP
#include <linux/ip.h>			//for struct iphdr
#include <linux/icmp.h>			//for icmphdr
#include <netdb.h>				//for gethostbyname	
#include <arpa/inet.h>			//for inet_ntoa
#include <string.h>				//for strcpy
#include <sys/time.h>			//for select, gettimeofday
#include <unistd.h>				//for select


#define IPADDRBUFSIZE 50
#define DEBUG 0
#define debugLog(...) 	({\
							if(DEBUG==1){\
								/*printing debug information in cyan colour*/\
								printf("\033[0;36m");\
								printf(__VA_ARGS__);\
								printf("\033[0m");\
							}\
						})
#define UDP_DATAGRAM_SIZE 100
#define MAXBUFFERSIZE 200
#define MAXHOP 30

//global variables
int udpheaderlen, ipheaderlen, icmpheaderlen, payloadSize = 52;
int ttl;

int findIPAddress(char*, char*);
void generateRandomPayload(char*, int);
void setIPHeaderFields(struct iphdr *, struct sockaddr_in);
void setUDPHeaderFields(struct udphdr *, int, int);


// Finds out the IP address corresponding to the given domain name
int findIPAddress(char *domainName, char *ipAddrBuf){

	//to store gethostbyname returned structure address list
	struct in_addr **addrList;

	//declaring a temp structure to hold return value of gethostbyname function
	struct hostent *tempStructure;
	tempStructure = gethostbyname(domainName);

	if(tempStructure==NULL){
		return -1;
	}

	addrList = (struct in_addr **) tempStructure->h_addr_list;
	
	//ipAddrBuf will contain the ip address of the entered domain in IPv4 dotted-decimal notation
	strcpy(ipAddrBuf, inet_ntoa(*addrList[0]));

	return 0;
}

//generate the payload with random bytes for UDP Packet
void generateRandomPayload(char* payload, int size){
	
	int i;

	for(i=0; i<size-1; i++){
		//random integer from 1 to 255
		payload[i] = rand()%255 + 1;
	}

	payload[i] = '\0';
}

void setIPHeaderFields(struct iphdr *iph, struct sockaddr_in daddr){

	//ihl: the ip header length in 32bit octets. this means a value of 5 for the hl means 20 bytes (5 * 4)
	iph->ihl = 5;

	//version: the ip version is always 4
	iph->version = 4;

	//tos: type of service controls the priority of the packet. 0x00 is normal
	iph->tos = 0;

	//tot_len: total length must contain the total length of the ip datagram. this includes ip header, udp header and payload size in bytes
	iph->tot_len = ipheaderlen + udpheaderlen + payloadSize;

	//id: the id sequence number is mainly used for reassembly of fragmented IP datagrams
	iph->id = htonl(54321);

	// frag_off: the fragment offset is used for reassembly of fragmented datagrams
	iph->frag_off = 0;

	//ttl: time to live is the amount of hops (routers to pass) before the packet is discarded, and an icmp error message is returned. the maximum is 255.
	iph->ttl = ttl;

	//protocol: can be tcp (6), udp(17), icmp(1)
	iph->protocol = 17;

	//check: the datagram checksum for the whole ip datagram, setting 0 for now
	iph->check = 0;

	//ip_src and ip_dst: source and destination IP address, converted to long format, e.g. by inet_addr()
	iph->saddr = INADDR_ANY;
	iph->daddr = daddr.sin_addr.s_addr;

}

void setUDPHeaderFields(struct udphdr *udph, int sourcePort, int destPort){

		//source port
		udph->source = sourcePort;

		udph->dest = destPort;

		//udpheader length
		udph->len = htons(udpheaderlen + payloadSize);

		//not using udp checksum for now
		udph->check = 0;
}


int main(int argc, char *argv[]){

	char ipAddrBuf[IPADDRBUFSIZE];
	
	// Checks number of command line arguments
	if(argc!=2){
		perror("Insufficient number of inputs provided, the correct format is \"mytraceroute <destination domain name>\"");
		exit(-1);
	}

	// Finds out the IP address corresponding to the given domain name
	if(findIPAddress(argv[1], ipAddrBuf)<0){
		perror("Invalid Domain Name Error!");
		exit(-1);
	}

	debugLog("IP Address for given domain name : %s\n",ipAddrBuf);

	// Creates the two raw sockets with appropriate parameters
	int S1fd, S2fd;

	//S1 : To send UDP packets
	S1fd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);

	if(S1fd < 0){
		perror("Creation of socket failed for S1");
		exit(-1);
	}

	//S2 : To receive ICMP packets
	S2fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	if(S2fd < 0){
		perror("Creation of socket failed for S2");
		exit(-1);
	}

	// Specifies socket addresses
	struct sockaddr_in saddr_S1, saddr_S2, daddr, icmpRecvAddr;
	int saddr_S1_len, saddr_S2_len, daddr_len, icmpRecvAddr_len;

	saddr_S1.sin_family = AF_INET;
	saddr_S1.sin_port = htons(8182);
	saddr_S1.sin_addr.s_addr = INADDR_ANY; 
	saddr_S1_len = sizeof(saddr_S1);

	saddr_S2.sin_family = AF_INET;
	saddr_S2.sin_port = htons(8183);
	saddr_S2.sin_addr.s_addr = INADDR_ANY; 
	saddr_S2_len = sizeof(saddr_S2);

	daddr.sin_family = AF_INET;
	//In the UDP packet, set the destination port as 32164.
	daddr.sin_port = htons(32164); 
	inet_aton(ipAddrBuf, &daddr.sin_addr);
	daddr_len = sizeof(daddr);

	icmpRecvAddr_len = sizeof(icmpRecvAddr_len);

	// Binds S1 to local IP
	if(bind(S1fd, (struct sockaddr*) &saddr_S1, saddr_S1_len) < 0){
		perror("Error in binding socket S1");
		exit(-1);
	}

	// Binds S2 to local IP
	if(bind(S2fd, (struct sockaddr*) &saddr_S2, saddr_S2_len) < 0){
		perror("Error in binding socket S2");
		exit(-1);
	}

	int on = 1;

	// Sets the socket option on S1 to include IP_HDRINCL.
	if(setsockopt(S1fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0){
		perror("Error in setting socket option on S1 failed!");
		exit(-1);
	}

	char udpPayload[52];
	char datagram[UDP_DATAGRAM_SIZE];
	struct udphdr *udph;
	struct iphdr *iph, *iphReceived;
	struct icmphdr *icmph;
	int nfds, i, completed = 0, repeats = 0;
	int setTimeout = 1;
 	fd_set readfs;
 	struct timeval timeOut;
 	int numFileDescr;
 	char icmp_buffer[MAXBUFFERSIZE];
 	struct timeval timeCal1, timeCal2;

	udpheaderlen = sizeof(struct udphdr);
	ipheaderlen = sizeof(struct iphdr);
	icmpheaderlen = sizeof(struct icmphdr);

	// Sends a UDP packet for traceroute using S1:
	// 	Sets the variable TTL = 1.
	ttl = 1;

	// 	Creates a UDP datagram with a payload size of 52 bytes.

	printf("Hop Count\tSrc IP\t\t\tResponse Time\t\tMax Hops : %d\n",MAXHOP);

	while(1){
		//Generate the payload with random bytes. 
		generateRandomPayload(udpPayload, payloadSize);
		debugLog("Payload : %s\n", udpPayload);
		debugLog("Payload Size including NULL character : %d\n", (int)(strlen(udpPayload)) + 1);
		
		//Set the header fields correctly.
		//See /usr/include/linux/udp.h and /usr/include/linux/ip.h
		
		//empty the buffer initially
		memset(datagram, 0, UDP_DATAGRAM_SIZE);

		// 	Appends an IP header with the UDP datagram.
		//  point ip header to the beginning of the datagram
		iph = (struct iphdr *)datagram;

		// 	Sets the fields of the IP headers properly, including the TTL field.
		setIPHeaderFields(iph, daddr);

		//udpheader points to the ending of ipheader
		udph = (struct udphdr *)(datagram + ipheaderlen);

		// 	Sets the fields of the UDP headers properly, including the TTL field.
		setUDPHeaderFields(udph, saddr_S1.sin_port, daddr.sin_port);

		//copying payload to appropriate location
		memcpy(datagram + ipheaderlen + udpheaderlen, udpPayload, payloadSize);

		debugLog("Datagram : ");
		for(i=0; i<100; i++){
			debugLog("%c",datagram[i]);
		}
		debugLog("\n");

		//sends the packet through the raw socket S1
		sendto(S1fd, datagram, ipheaderlen + udpheaderlen + payloadSize, 0,(const struct sockaddr *)&daddr, daddr_len);
		
		gettimeofday(&timeCal1,NULL);

	 	debugLog("UDP Packet Sent\n");

	 	while(1){
		 	//Receive using S2

		 	FD_ZERO(&readfs);
		 	FD_SET(S2fd, &readfs);

		 	if(setTimeout){
		 		//setting timeout value of 1 sec
		 		timeOut.tv_sec = 1;
		 		timeOut.tv_usec = 0;
		 	}
		 	else{
		 		setTimeout = 1;
		 	}

		 	nfds = S2fd+1;

		 	// Makes a select call to wait for a ICMP message to be received using the raw socket S2 or a timeout value of 1 sec
		 	numFileDescr = select(nfds, &readfs, NULL, NULL, &timeOut);

			if(numFileDescr > 0){
				// 	Comes out with receive of an ICMP message in S2

				//empty the buffer initially
				memset(icmp_buffer, 0, MAXBUFFERSIZE);

			    recvfrom(S2fd, (char *)icmp_buffer, MAXBUFFERSIZE, 0, ( struct sockaddr *) &icmpRecvAddr, &icmpRecvAddr_len); 

			    debugLog("\nReceived : ");
			    for(i=0; i<MAXBUFFERSIZE; i++){
					debugLog("%c",icmp_buffer[i]);
				}
				debugLog("\n\n");

				//Needs to check the IP protocol field to identify a ICMP packet which has a protocol number 1
				iphReceived = (struct iphdr *)(icmp_buffer);

				debugLog("IP Protocol Field : %d\n", iphReceived->protocol);

				//protocol should be 1 which is ICMP
				if(iphReceived->protocol == 1){
					//Point icmp header to the beginning + ipheaderlen of the received message 
					icmph = (struct icmphdr *)(icmp_buffer + (iphReceived->ihl)*4);		

					debugLog("Received ICMP Type: %d\n",icmph->type);

					struct in_addr sourceIPAddress;
					char sourceIPAddressBuffer[30];
					sourceIPAddress.s_addr = iphReceived->saddr;
					strcpy(sourceIPAddressBuffer, inet_ntoa(sourceIPAddress));

					//Checks the ICMP header to check the type field
					if(icmph->type == 3 && sourceIPAddress.s_addr == (daddr.sin_addr).s_addr){
						
						gettimeofday(&timeCal2,NULL);

						//it is a ICMP Destination Unreachable Message
						//Checked that the source IP address of the ICMP Destination Unreachable Message matches with our target server IP address
						//Reached to the target destination, and received the response from there.
						
						//Prints 
						printf("%d\t\t%s\t\t%lf ms\n",ttl, sourceIPAddressBuffer, (timeCal2.tv_sec-timeCal1.tv_sec)*1000 + (timeCal2.tv_usec*1.0 - timeCal1.tv_usec*1.0)/1000.0);

						debugLog("Source IP : %d\n",sourceIPAddress.s_addr);
						debugLog("Destination IP : %d\n",(daddr.sin_addr).s_addr);

						//program execution is completed
						completed = 1;

						//breaks from the select while loop
						break;
										
					}
					else if(icmph->type==11){

						gettimeofday(&timeCal2,NULL);

						//It is an ICMP Time Exceeded Message and we have got the response from the Layer 3 device at an intermediate hop specified by the TTL value  
						debugLog("Source IP : %s\n",sourceIPAddressBuffer);
						
						//Have identified the device IP at a hop specified by the TTL value.
						//Prints
						printf("%d\t\t%s\t\t%lf ms\n",ttl, sourceIPAddressBuffer, (timeCal2.tv_sec-timeCal1.tv_sec)*1000 + (timeCal2.tv_usec*1.0 - timeCal1.tv_usec*1.0)/1000.0);

						// Increments the TTL value by 1 and sends the UDP packet with this new TTL value. 
						ttl++;
						repeats = 0;

						//breaks from the select while loop
						break;
						
					}
					else{
					// 	Else if you have received any other ICMP packet, 
						debugLog("Received some other ICMP packet... Ignoring...");
					// 	Ignores and goes back to wait on the select call with the remaining value of timeout
						setTimeout = 0;
					}


					// For each ICMP Time Exceeded or Destination Unreachable received:
					// measures the response time = time difference between the UDP packet sent, measured just after the sendto() function, and the reception of the ICMP Time Exceeded message or Destination Unreachable, measured after the recvfrom() call).
					// 		Printing Format:
					// 			Hop_Count(TTL_Value)	Source IP address	Response_Time

				}
				else{
					//some other type of message (not icmp) is received
					debugLog("Received some other packet... Ignoring...");

					//go back on select call with remaining value of timeout
					setTimeout = 0;
				}
			}
			else{
				// 	If the select call times out,
				repeats++;

				// Total number of repeats with the same TTL is 3. 
				if(repeats == 3){

					// If the timeout occurs for all the three UDP packets and we do not receive a ICMP Time Exceeded Message or ICMP Destination Unreachable message in any of them, then print it as follows:
					
					// Hop_Count(TTL_Value)	*	*
					printf("%d\t\t*\t\t\t*\n",ttl);
					
					// Increments the TTL value by 1 and sends the UDP packet with this new TTL value. 
					ttl++;
					repeats = 0;

				}

				//Sends the UDP packet again.
				//breaks from the select while loop
				break;
			}
		}

		if(ttl-1 == MAXHOP)
			completed = 1;

		//break from the outer while loop if it is completed
		if(completed)
			break;

		//send UDP packet again with the help of the outer while loop

	}

	//closing sockets
	close(S1fd);
	close(S2fd);

	return 0;
}