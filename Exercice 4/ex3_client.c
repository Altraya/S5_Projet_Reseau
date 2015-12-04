
/*======================================================
			client.c
			Bit Alterné
./client <fichier_a_envoyer> <fichier_recu> <adr_IP_dist> <port_dist> [<port_local>]
./client fichierAEnvoye.txt fichierRecu.txt 192.168.132.128 5000
 ======================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include "timer.h"
#define BUFFER_LENGTH 1024

typedef struct message_s
{
	char buf[BUFFER_LENGTH];
	short int fin;
	int taille; // en octets
	short int bit;
	short int ack; // vaut 1 si c'est un ack
}message;

message* initMessage()
{
	message* m = malloc(sizeof(struct message_s));
	m->fin = 0;
	m->bit = 0;
	return m;
}

int main(int argc, char *argv[])
{
	
	int input_fd;
	int output_fd;
	int nbchar;
	if (argc < 5)
	{
		fprintf(stderr, "Usage: %s <fichier_a_envoyer> <fichier_recu> <adr_IP_dist> <port_dist> [<port_local>]\n", argv[0]);
		exit(1);
	};
	char* nomFichierAEnvoyer = argv[1];
	char* nomFichierRecu = argv[2];
	char* adresseIp = argv[3];
	int port = atoi(argv[4]);
	int portLocal = 0;
	if(argc > 5)
		portLocal = atoi(argv[5]);
	
	//Adresse distante
	struct sockaddr_in adr;
	adr.sin_family = AF_INET;
	inet_pton(AF_INET, adresseIp, &(adr.sin_addr));
	adr.sin_port = htons(port);
	printf("Ip serveur : %s\n", inet_ntoa(adr.sin_addr));
	printf("Port distant : %d\n", ntohs(adr.sin_port));

	//Adresse locale
	struct sockaddr_in adrLocale;
	adrLocale.sin_family = AF_INET;
	//adrLocale.sin_addr.s_addr = htonl(INADDR_ANY);
	inet_pton(AF_INET, "127.0.0.1", &(adrLocale.sin_addr));

	if(argc > 5) //regarde le nombre d'argument pour specifier le port ou pour laisser bind l'attribuer
		adrLocale.sin_port = htons(portLocal);
	else
		adrLocale.sin_port = htons(0); //si on met le port a 0 bind attribura automatiquement un des ports

	printf("Ip client : %s\n", inet_ntoa(adrLocale.sin_addr));
	printf("Port local : %d\n", ntohs(adrLocale.sin_port));

	int fd;
	fd=socket(AF_INET,SOCK_DGRAM,0);
	if(fd==-1)
	{
		perror("socket");
		exit(1);
	}


	// ouvrir le fichier a envoyer en lecture
	input_fd = open(nomFichierAEnvoyer, O_RDONLY);
	if(input_fd<0)
	{	
		perror("open input"); 
		exit(1); 		
	}

	// ouvrir le fichier en ecriture que l'on recoit
	output_fd = open(nomFichierRecu, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	if(output_fd<0)
	{	
		perror("open output"); 
		exit(1); 		
	}
	short int bit_co = 0;
	socklen_t addrLocale = sizeof(adrLocale);

	socklen_t a = sizeof(adr);
	message* buffConnexion = initMessage();
	message* ackConnexion = initMessage();
	buffConnexion->bit = bit_co;
	buffConnexion->ack = 0;
	int nbCharCon = 0;
	int caca = 0;
	while(caca==0)
	{
		fd_set lala;
	   int r;
	   struct timeval conn;

	   // Initialize the set
	   FD_ZERO(&lala);
	   FD_SET(fd, &lala);
	   
	   // Initialize time out struct
	   conn.tv_sec = 0;
	   conn.tv_usec = 100 * 1000;

		// Check status
	  
	    	printf("Connexion envoi d'un datagramme de connexion.\n");
		nbCharCon = sendto(fd, buffConnexion, 0, 0, (struct sockaddr*)&adr, a);
		r = select(fd+1, &lala, NULL, NULL, &conn);
		 if (r < 0)
	    {
	      	perror("select");
	      	exit(1);
	    }
	    printf("on lance le timer\n");
		if(FD_ISSET(fd, &lala))
		{
			printf("S'il s'est passé qqchose sur le socket CO\n");
			//on cherche à recevoir un ack
			recvfrom(fd, ackConnexion, sizeof(message), 0, (struct sockaddr*)&adrLocale, &addrLocale);
			//tester si le ack est le bon 
			if(ackConnexion->ack==1 && ackConnexion->bit==bit_co)
			{
				//si c'est le bon ack on change le bit.
				caca = 1;
				printf("on est connecté ! :D\n");
				//et on lit la suite du message
			}
			else
			{
				printf("(on n'a pas le bon ack, WTF)\n");
			}

		}
		else if(r==0)
		{
			printf("timeout du msg de co!!!\n");
			//on réitère l'envoi

		}
	}


	message* messageRecu = initMessage();
	message* messageAEnvoyer = initMessage();
	int nbLuEnvoi = 0;
	int nbLuRecoi = 0;
	short int bit_v = 0; //bit alterné
	int lireLaSuite = 0;

    // tant qu'on a à envoyer et à recevoir
	while(!(messageAEnvoyer->fin) && !(messageRecu->fin))
	{
		//tant qu'on a à envoyer on remet le timeout à zéro
		if(!messageAEnvoyer->fin)
		{
			fd_set readset;
		   int result;
		   struct timeval tv;

		   // Initialize the set
		   FD_ZERO(&readset);
		   FD_SET(fd, &readset);
		   
		   // Initialize time out struct
		   tv.tv_sec = 0;
		   tv.tv_usec = 1000 * 1000;

	   		// Check status
		  
			printf("Coté client : Encore des messages a envoyer \n");
			messageAEnvoyer->bit = bit_v;
			printf("lireLaSuite = %d\n", lireLaSuite);
			if(lireLaSuite==0)
			{
				nbLuEnvoi = read(input_fd, messageAEnvoyer->buf, BUFFER_LENGTH);
				messageAEnvoyer->taille=nbLuEnvoi;
				if((nbLuEnvoi < BUFFER_LENGTH))
				{
					messageAEnvoyer->fin=1;
					printf("Coté client plus de message a envoyer | %d\n", messageAEnvoyer->fin);
				}
			}
			printf("On envoie un message.\n");
			nbchar=sendto(fd, messageAEnvoyer, sizeof(message), 0, (struct sockaddr*)&adr, a);
			result = select(fd+1, &readset, NULL, NULL, &tv);
			 if (result < 0)
		   {
		      	perror("select");
		      	exit(1);
		   }

			if(FD_ISSET(fd, &readset))
			{
				printf("S'il s'est passé qqchose sur le socket \n");
				//on cherche à recevoir un ack
				recvfrom(fd, messageRecu, sizeof(message), 0, (struct sockaddr*)&adrLocale, &addrLocale);
				//tester si le ack est le bon TODO
				if(messageRecu->ack==1 && messageRecu->bit==bit_v)
				{
					printf("on a reçu le bon ack.\n");
					//si c'est le bon ack on change le bit.
					(bit_v+=1)%2;
					//et on lit la suite du message
					lireLaSuite = 0;
				}
				else
				{
					printf("(on n'a pas le bon ack, WTF)\n");
				}

			}
			else if(result==0)
			{
				printf("timeout !!!\n");
				lireLaSuite=1;
				//on réitère l'envoi

			}
			printf("Le client a envoyé %d octets \n",nbchar);
			if(messageAEnvoyer->fin)
				printf("Le client envoie une demande de fermeture de connexion\n");
		}

		if(!messageRecu->fin)
		{
			printf("Coté client : Encore des messages a recevoir \n");
			nbLuRecoi = recvfrom(fd, messageRecu, sizeof(message), 0, (struct sockaddr*)&adrLocale, &addrLocale);
			printf("Coté client message recu fin | %d\n", messageRecu->fin);
			printf("Le client a recu %d octets\n", nbLuRecoi);
			if(messageRecu->fin)
				printf("Le client ferme la connexion l'émetteur (serveur) a envoyé une demande de fermeture\n");
			write(output_fd, messageRecu->buf, messageRecu->taille);
		}

		
				
	}	
	close(input_fd);
	close(output_fd);
	return 0;
}

/*
utiliser un select ou un setsockopt pour gérer le timeout
*/
