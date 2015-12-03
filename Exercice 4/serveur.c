
/*======================================================
			serveur.c
			Bit Alterné
./serveur <fichier_a_envoyer> <fichier_recu> <port_locale>
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
	int taille;
	short int bit;
}message;

message* initMessage()
{
	message* m = malloc(sizeof(struct message_s));
	m->fin=0;
	return m;
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

	char buffConnexion[0];
	int nbCharCon = 0;
	socklen_t b = sizeof(adrDist);

	printf("En attente d'une connexion \n");
	nbCharCon = recvfrom(fd, buffConnexion, 0, 0, (struct sockaddr*)&adrDist, &b);
	printf("Connexion en cours : Le serveur a recu un datagramme de : %d octets\n", nbCharCon);
	
	printf("Ip adresse locale : %s\n", inet_ntoa(adrLocale.sin_addr));
	printf("Port locale %d\n", ntohs(adrLocale.sin_port));


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
	int nbLuEnvoi = 0;
	int nbLuRecoi = 0;
	socklen_t addrDist = sizeof(adrDist);
	socklen_t addrLocale = sizeof(adrLocale);
	short int ack_v = 0;
	while(!(messageRecu->fin && messageAEnvoyer->fin))
	{
		//tant qu'il y a des messages à recevoir
		if(!messageRecu->fin)
		{
			nbLuRecoi = recvfrom(fd, messageRecu, sizeof(message), 0,(struct sockaddr*)&adrLocale, &addrLocale);
			//quand on reçoit un message on doit envoyer le ack.
			printf("Le serveur a recu %d octets \n", nbLuRecoi);
			msg_ack->bit = messageRecu->bit;
			msg_ack->ack = 1;
			msg_ack->buf = "ceci est un ack :D";
			//si jamais le ack se perd malencontreusement, le client va renvoyer le même message
			//qu'il faudra ignorer par la suite.
			sendto(fd, msg_ack, sizeof(message), 0, (struct sockaddr*)&adrDist, addrDist);
			if(messageRecu->fin)
				printf("Le serveur ferme la connexion l'émetteur (client) a envoyé une demande de fermeture\n");
			//si on a bien la trame attendue on consomme les données
			if(messageRecu->bit==ack_v)
			{
				write(output_fd, messageRecu->buf, messageRecu->taille);
			}
			//sinon, on s'en branle

			//on actualise le numéro de la prochaine trame attendue
			(ack_v+=1)%2; 
		}

		if(!messageAEnvoyer->fin)
		{
			nbLuEnvoi = read(input_fd, messageAEnvoyer->buf, BUFFER_LENGTH);
			messageAEnvoyer->taille=nbLuEnvoi;
			if(nbLuEnvoi < BUFFER_LENGTH)
			{
				messageAEnvoyer->fin=1;
				printf("Le serveur envoi une demande de fermeture de connexion \n");			
			}
			
			nbchar=sendto(fd, messageAEnvoyer, sizeof(message), 0, (struct sockaddr*)&adrDist, addrDist);
			printf("Le serveur envoi %d octets \n", nbchar);
		}

	}
	close(input_fd);
	close(output_fd);
	return 0;
}
