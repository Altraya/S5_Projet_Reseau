
/*======================================================
			serveur.c
			Bit Alterné
./serveur <fichier_a_envoyer> <fichier_recu> <port_local>
./serveur envoieServ.txt recuServ.txt 5000
 ======================================================*/

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
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
	short int flagEnPlus; // vaut 0 quand on fait un échange
						  // du client au serveur. 1 sinon
	short int connexion; //vaut 1 si c'est le datagramme de connexion
}message;

message* initMessage()
{
	message* m = malloc(sizeof(struct message_s));
	m->fin = 0;
	m->bit = 0;
	return m;
}

void printMessage (message* mess, char* nomMess)
{
	printf( "%s :\t fin=%d\n \t bit=%d\n\tack=%d\n\tflagEnPlus=%d\n\tconnexion=%d\n\tbuf=%s\n",nomMess,mess->fin,mess->bit,mess->ack,mess->flagEnPlus,mess->connexion,mess->buf);	
	printf("\n");
}

int main(int argc, char **argv)
{
	if (argc < 4)
	{
		fprintf(stderr, "Usage: %s <fichier_a_envoyer>  <fichier_recu> <port_local>\n", argv[0]);
		exit(1);
	}
	char* nomFichierAEnvoyer = argv[1];
	char* nomFichierRecu = argv[2];
	printf("Nom fichier a envoye: %s\n", nomFichierAEnvoyer);
	printf("Nom fichier recu %s\n", nomFichierRecu);
	int port = atoi(argv[3]);
	printf("Port : %d\n", port);

	int input_fd;
	int output_fd;
	int nbchar;

	//Adresse locale
	struct sockaddr_in adrLocale;
	adrLocale.sin_family = AF_INET;
	adrLocale.sin_addr.s_addr = htonl(INADDR_ANY);
	adrLocale.sin_port = htons(port); 

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
	
	//Connexion : adresse du client qui nous a contacté
	struct sockaddr_in adrDist;
	adrDist.sin_family = AF_INET;

	message* buffConnexion = initMessage();
	message* ack_conn = initMessage();
	ack_conn->connexion = 1;
	ack_conn->ack = 1;
	int nbCharCon = 0;
	socklen_t addrLocale = sizeof(adrLocale);
	socklen_t addrDist = sizeof(adrDist);

	printf("En attente d'une connexion \n");
	nbCharCon = recvfrom(fd, buffConnexion, sizeof(message), 0, (struct sockaddr*)&adrLocale, &addrLocale);
	nbCharCon = sendto(fd, buffConnexion, sizeof(message), 0, (struct sockaddr*)&adrDist, addrDist);
	printMessage(buffConnexion,"je t'envoie ack connexion");
	printf("Ip adresse locale : %s\n", inet_ntoa(adrLocale.sin_addr));
	printf("Port local %d\n", ntohs(adrLocale.sin_port));


	printf("Ipclient : %s \n", inet_ntoa(adrDist.sin_addr));
	printf("Port client %d\n", ntohs(adrDist.sin_port));

	printf("Connexion réussie \n");
	
	// ouvrir le fichier a envoyer en lecture
	input_fd = open(nomFichierAEnvoyer, O_RDONLY);
	if(input_fd<0)
	{	
		perror("open input"); 
		exit(1); 		
	}
	printf("Ouverture du fichier %s\n", nomFichierAEnvoyer);

	//check si le fichier existe déjà pour le delete et pas avoir une erreur => plus pratique pour les tests
	// ouvrir le fichier en ecriture que l'on recoit
	output_fd = open(nomFichierRecu, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	if(output_fd<0)
	{	
		perror("open output");
		exit(1); 		
	}
	printf("Ouverture du fichier nomFichierRecu %s\n", nomFichierRecu);

	// copie :
	message* messageRecu = initMessage();
	message* messageAEnvoyer = initMessage();
	message* msg_ack = initMessage();
	msg_ack->ack=1;
	int nbLuEnvoi = 0;
	int nbLuRecoi = 0;
	short int lireLaSuite = 0;
	short int ecrireLaSuite = 0;
	short int bit_attendu = 0;
	short int bit_a_envoyer = 0;
	fd_set readset_c;
	int result_c;
	struct timeval tv_c;

	while(!(messageRecu->fin && messageAEnvoyer->fin))
	{

		if(!messageRecu->fin)
		{
			nbLuRecoi = recvfrom(fd, messageRecu, sizeof(message), 0,(struct sockaddr*)&adrLocale, &addrLocale);
			printf("Le serveur a recu %d octets \n", nbLuRecoi);
			printMessage(messageRecu,"messageRecu");
			if(messageRecu->connexion==1)
			{
				printf("je t'envoie le ack de connexion.\n");
				sendto(fd,buffConnexion,sizeof(message),0,(struct sockaddr*)&adrDist, addrDist);
			}
			else
			{
				if(messageRecu->fin)
				{
					printf("Le serveur ferme la connexion l'émetteur (client) a envoyé une demande de fermeture\n");
				}
				if(messageRecu->ack==1)
				{
					lireLaSuite=0;
					bit_a_envoyer=(bit_a_envoyer+1)%2;
				}
				if(messageRecu->ack==0 && messageRecu->bit!=bit_attendu)
				{
					ecrireLaSuite=1;
					//envoi ack		
					sendto(fd,msg_ack,sizeof(message),0,(struct sockaddr*)&adrDist, addrDist);
				}
				else if(messageRecu->ack==0 && messageRecu->bit==bit_attendu)
				{
					ecrireLaSuite=0;
					//envoi ack
					sendto(fd,msg_ack,sizeof(message),0,(struct sockaddr*)&adrDist, addrDist);
					bit_attendu=(bit_attendu+1)%2;

				}
				if(messageRecu->ack==0 && ecrireLaSuite==0)	
					write(output_fd, messageRecu->buf, messageRecu->taille);
			} 
		}

		if(!messageAEnvoyer->fin)
		{
			
			// Initialize the set
			FD_ZERO(&readset_c);
			FD_SET(fd, &readset_c);

			// Initialize time out struct
			tv_c.tv_sec = 0;
			tv_c.tv_usec = 1000 * 1000;

			if(lireLaSuite == 0)
			{
				nbLuEnvoi = read(input_fd, messageAEnvoyer->buf, BUFFER_LENGTH);
				messageAEnvoyer->taille=nbLuEnvoi;
				if(nbLuEnvoi < BUFFER_LENGTH)
				{
					messageAEnvoyer->fin=1;
					printf("Le serveur envoi une demande de fermeture de connexion \n");			
				}
			}
			
			nbchar=sendto(fd, messageAEnvoyer, sizeof(message), 0, (struct sockaddr*)&adrDist, addrDist);
			bit_attendu=(bit_attendu+1)%2;	
			printf("Le serveur envoi %d octets \n", nbchar);
			result_c = select(fd+1, &readset_c, NULL, NULL, &tv_c);
			if(result_c <0)
			{
				perror("select");
				exit(2);
			}
			if(FD_ISSET(fd, &readset_c))
			{
				nbCharCon = recvfrom(fd, messageRecu, sizeof(message), 0, (struct sockaddr*)&adrLocale, &addrLocale);
				if(messageRecu->connexion==1)
				{
					printf("gros bordel de merde.\n");
					sendto(fd,buffConnexion,sizeof(message),0,(struct sockaddr*)&adrDist, addrDist);
				}
				else
				{
					if(messageRecu->fin)
					{
						printf("Le serveur ferme la connexion l'émetteur (client) a envoyé une demande de fermeture\n");
					}
					if(messageRecu->ack==1)
					{
						lireLaSuite=0;
						bit_a_envoyer=(bit_a_envoyer+1)%2;
					}
					if(messageRecu->ack==0 && messageRecu->bit!=bit_attendu)
					{
						ecrireLaSuite=1;
						//envoi ack		
						sendto(fd,msg_ack,sizeof(message),0,(struct sockaddr*)&adrDist, addrDist);
					}
					else if(messageRecu->ack==0 && messageRecu->bit==bit_attendu)
					{
						ecrireLaSuite=0;
						//envoi ack
						sendto(fd,msg_ack,sizeof(message),0,(struct sockaddr*)&adrDist, addrDist);
						bit_attendu=(bit_attendu+1)%2;

					}
					if(messageRecu->ack==0 && ecrireLaSuite==0)	
						write(output_fd, messageRecu->buf, messageRecu->taille);
				} 
			}
			if(result_c==0)
			{
				lireLaSuite=1;
			}
		}
		

	}
	close(input_fd);
	close(output_fd);
	return 0;
}
