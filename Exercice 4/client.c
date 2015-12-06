
/*======================================================
			client.c
			Bit alterné
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
#define BUFFER_LENGTH 1024

typedef struct message_s
{
	char buf[BUFFER_LENGTH];
	short int fin;
	int taille; // en octets
	short int bit;
	short int ack; // vaut 1 si c'est un ack
	short int connexion; //vaut 1 si c'est le datagramme de connexion
}message;

message* initMessage()
{
	message* m = malloc(sizeof(struct message_s));
	m->fin = 0;
	m->bit = 0;
	m->taille = 0;
	m->ack = 0;
	m->flagEnPlus = 0;
	m->connexion = 0;
	return m;
}

void printMessage (message* mess, char* nomMess)
{
	printf( "%s : fin=%d\n bit=%d\nack=%d\n connexion=%d\nbuf=%s\n",nomMess,mess->fin,mess->bit,mess->ack,mess->connexion,mess->buf);	
	printf("\n");
}

int main(int argc, char *argv[])
{
	int input_fd;
	int output_fd;
	int nbchar;
	if (argc < 5){
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
	adrLocale.sin_addr.s_addr = htonl(INADDR_ANY);

	if(argc > 5) //regarde le nombre d'argument pour specifier le port ou pour laisser bind l'attribuer
		adrLocale.sin_port = htons(portLocal);
	else
		adrLocale.sin_port = htons(0); //si on met le port a 0 bind attribura automatiquement un des ports

	printf("Ip client : %s\n", inet_ntoa(adrLocale.sin_addr));

	int fd;
	fd=socket(AF_INET,SOCK_DGRAM,0);
	if(fd==-1)
	{
		perror("socket");
		exit(1);
	}

	if(bind(fd,(struct sockaddr*)&adrLocale,sizeof(adrLocale))==-1)
	{
		perror("bind");
		exit(2);
	}
	printf("Port local : %d\n", ntohs(adrLocale.sin_port));

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

	socklen_t addrLocale = sizeof(adrLocale);
	socklen_t addrDist = sizeof(adr);
	message* buffConnexion = initMessage();
	buffConnexion->connexion=1;
	int nbCharCon = 0;
	

	int sortie = 1;
	while(sortie)
	{
		fd_set readset_c;
		int result_c;
		struct timeval tv_c;
		// Initialize the set
		FD_ZERO(&readset_c);
		FD_SET(fd, &readset_c);

		// Initialize time out struct
		tv_c.tv_sec = 0;
		tv_c.tv_usec = 2000 * 1000;

		nbCharCon = sendto(fd, buffConnexion, sizeof(message), 0, (struct sockaddr*)&adr, addrDist);
		result_c = select(fd+1, &readset_c, NULL, NULL, &tv_c);
		printf("Connexion envoi d'un datagramme de %d caractère\n", nbCharCon);
		printMessage(buffConnexion,"buffConnexion");
		 if (result_c < 0)
	    {
	      	perror("select");
	      	exit(1);
	    }
		if(FD_ISSET(fd, &readset_c))
		{
			printf("Il s'est passé qqchose sur le socket CO \n");
			recvfrom(fd, buffConnexion, sizeof(message), 0, (struct sockaddr*)&adrLocale, &addrLocale);
			printf("on est connecté ! :)\n");
			sortie=0;
		}
		else if(result_c==0)
		{
			printf("timeout!!\n");
		}

	}

	message* messageRecu = initMessage();
	message* messageAEnvoyer = initMessage();
	message* msg_ack = initMessage();
	msg_ack->ack=1;
	int nbLuEnvoi = 0;
	int nbLuRecoi = 0;
	int finished = 0;
	int finished2 = 0;
	short int lireLaSuite = 0;
	short int ecrireLaSuite = 0;
	short int bit_attendu = 0;
	short int bit_a_envoyer = 0;


	while(finished2==0 && finished==0)
	{
//envoi
		if(finished2==0)
		{
			messageAEnvoyer->bit=bit_a_envoyer;
			fd_set readset;
			int result;
			struct timeval tv;

			// Initialize the set
			FD_ZERO(&readset);
			FD_SET(fd, &readset);

			// Initialize time out struct
			tv.tv_sec = 0;
			tv.tv_usec = 2000 * 1000;

			printf("Coté client : Encore des messages a envoyer \n");
			if(lireLaSuite==0)
			{
				printf("on lit la suite du message.\n");
				nbLuEnvoi = read(input_fd, messageAEnvoyer->buf, BUFFER_LENGTH);
				messageAEnvoyer->taille=nbLuEnvoi;
				printf("le client envoie dans boucle 1\n");
				printMessage(messageAEnvoyer,"on envoie :");
				if(nbLuEnvoi < BUFFER_LENGTH)
				{
					messageAEnvoyer->fin=1;
					finished2=1;
					lireLaSuite=1;
					printf("Coté client : plus de message a envoyer | %d\n", messageAEnvoyer->fin);
				}
			}
			nbchar=sendto(fd, messageAEnvoyer, sizeof(message), 0, (struct sockaddr*)&adr, addrDist);
			printf("Le client a envoyé %d octets \n",nbchar);
			
			result = select(fd+1, &readset, NULL, NULL, &tv);
			printf("on lance le timer\n");
			 if (result < 0)
		    {
		      	perror("select");
		      	exit(1);
		    }
			if(FD_ISSET(fd, &readset))
			{
				recvfrom(fd, messageRecu, sizeof(message), 0, (struct sockaddr*)&adrLocale, &addrLocale);
				printf("on a reçu : \n");
				printMessage(messageRecu,"messageRecu");
				if(messageRecu->fin==1)
					finished=1;
				if(messageRecu->ack==1)
				{
					printf("client reçoit un ack. prochain msg à envoyer : %d\n",bit_a_envoyer);
					lireLaSuite=0;
					bit_a_envoyer=(bit_a_envoyer+1)%2;
				}
				if(messageRecu->ack==0 && messageRecu->bit!=bit_attendu)
				{
					printf("client reçoit un message mais pas le bon.\n");
					ecrireLaSuite=1;
					//envoi ack		
					printMessage(msg_ack,"on ack.");
					sendto(fd,msg_ack,sizeof(message),0,(struct sockaddr*)&adr, addrDist);
				}
				else if(messageRecu->ack==0 && messageRecu->bit==bit_attendu)
				{
					printf("Client reçoit le bon message");
					ecrireLaSuite=0;
					//envoi ack
					printMessage(msg_ack,"on envoie le ack msg_ack");
					sendto(fd,msg_ack,sizeof(message),0,(struct sockaddr*)&adr, addrDist);
					bit_attendu=(bit_attendu+1)%2;

				}
				if(messageRecu->ack==0 && ecrireLaSuite==0)	
				{
					write(output_fd, messageRecu->buf, messageRecu->taille);
					printf("On écrit des données.\n");
				}
					
			} 
			if(result==0)
			{
				printf("timeout.\n");
				lireLaSuite=1;
			}


		}
		//recoit
		if(finished==0)
		{
			nbLuRecoi = recvfrom(fd, messageRecu, sizeof(message), 0,(struct sockaddr*)&adrLocale, &addrLocale);
			printf("Client a recu %d octets \n", nbLuRecoi);
			printMessage(messageRecu,"messageRecu");
			if(messageRecu->connexion==1)
			{
				printf("on n'est pas supposé en arriver là.\n");
			}			
			else
			{
				if(messageRecu->fin==1)
				{
					finished=1;
					printf("Client ferme la connexion l'émetteur (serveur) a envoyé une demande de fermeture\n");
				}
				if(messageRecu->ack==1)
				{
					printf("client reçoit un ack :)2\n");
					lireLaSuite=0;
					bit_a_envoyer=(bit_a_envoyer+1)%2;
				}
				if(messageRecu->ack==0 && messageRecu->bit!=bit_attendu)
				{
					printf("client reçoit un message mais pas le bon2\n");
					ecrireLaSuite=1;
					//envoi ack		
					printMessage(msg_ack,"Envoi ack.");
					sendto(fd,msg_ack,sizeof(message),0,(struct sockaddr*)&adr, addrDist);
				}
				else if(messageRecu->ack==0 && messageRecu->bit==bit_attendu)
				{
					printf("client reçoit le bon message2 : %d\n",bit_attendu);
					ecrireLaSuite=0;
					//envoi ack
					printMessage(msg_ack,"ACK!");
					sendto(fd,msg_ack,sizeof(message),0,(struct sockaddr*)&adr, addrDist);
					bit_attendu=(bit_attendu+1)%2;

				}
				if(messageRecu->ack==0 && ecrireLaSuite==0)	
				{
					write(output_fd, messageRecu->buf, messageRecu->taille);
					printf("On consomme des données.\n");
				}
			} 
		}
	}	
	close(input_fd);
	close(output_fd);
	return 0;
}
