
/*======================================================*\
  Monday October the 1st 2012
  Arash HABIBI
  socket.h
\*======================================================*/

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>

#include <netinet/in.h>
#include <netinet/ip.h>  /* IP header */
#include <netinet/ip_icmp.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#include <netdb.h>

#define S_NAMES 100

// #define S_DOMAIN AF_INET
// #define S_DOMAIN AF_INET6
#define S_SOCKTYPE SOCK_DGRAM
// #define S_SOCKTYPE SOCK_RAW
// #define S_PROTOCOL IPPROTO_UDP
#define S_PROTOCOL 0
// #define S_PROTOCOL IPPROTO_RAW
// #define S_PROTOCOL IPPROTO_ICMP
// #define S_PROTOCOL 105

// #define S_MAX_MESSAGE 1024
#define S_MAX_MESSAGE 1024
#define BUFFER_LENGTH 1024


typedef struct message_s
{
	char buf[BUFFER_LENGTH];
	short int fin;
	int taille; // en octets
	short int bit;
	short int ack; // vaut 1 si c'est un ack
	short int flagEnPlus; // vaut 0 quand on fait un échange
						  // du client au serveur. 1 sinon
	short int connexion; //vaut 1 si c'est le datagramme de connexion
}messages;
int S_openAndBindSocket(int port);
int S_openSocket(void);
int S_distantAddress(char *IP_address, int port, struct sockaddr **address);
int S_sameAddress(struct sockaddr *address1, struct sockaddr *address2);
void S_humanReadableAddress(struct sockaddr *address, char *ip_address, int *port);

int S_receiveMessage(int sock_fd, struct sockaddr *adr, messages *message, int expected_length);
int S_sendMessage   (int sock_fd, struct sockaddr *adr, messages *message, int expected_length);

#endif

