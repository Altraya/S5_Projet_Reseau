
/*======================================================
			serveur.c
			GoBackN
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
#include "file.h"
#include "timer.h"

int main(int argc, char **argv)
{
	const int LARGEUR_FENETRE = 4;
	const int N = 5; // 8 + 1 : modulo
	const float timeout = 10.0;

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
	socklen_t addrDist = sizeof(adrDist);

	printf("En attente d'une connexion \n");
	nbCharCon = recvfrom(fd, buffConnexion, 0, 0, (struct sockaddr*)&adrDist, &addrDist);
	printf("Connexion en cours : Le serveur a recu un datagramme de : %d octets\n", nbCharCon);

	nbCharCon = 0;
	//initialise l'acquitement a 0
	ack* a = initAck();
	//envoie de l'acquitement 0 pour la connexion
	nbCharCon=sendto(fd, a, sizeof(ack), 0, (struct sockaddr*)&adrDist, addrDist);
	printf("Envoie de ack(%d)\n", a->numAck);
	
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
	int nbLuEnvoi = 0;
	int nbLuRecoi = 0;
	socklen_t addrLocale = sizeof(adrLocale);

	int PDAE = 0; // la borne sup de la fenêtre -1
	int PAA = 0; // Premier acquittement attendu (borne inf de la fenêtre d'émission)
	int DA = 0; 
	int i;
	//initialise notre fenêtre d'émission
	File* fenetreEmission = initialiser();

	printf("Envoie première fenêtre\n");
	//Envoie première fenêre
	for(i = 0; i != LARGEUR_FENETRE; i=(i+1)%N)
	{

		nbLuEnvoi = read(input_fd, messageAEnvoyer->buf, BUFFER_LENGTH);
		messageAEnvoyer->taille = nbLuEnvoi;
		messageAEnvoyer->seq = PDAE;
		messageAEnvoyer->ack = DA;
		if(nbLuEnvoi < BUFFER_LENGTH){
			messageAEnvoyer->fin=1;
			printf("Coté serveur plus de message a envoyer | %d\n", messageAEnvoyer->fin);
		}
		if(messageAEnvoyer-> taille > 0)
			nbchar=sendto(fd, messageAEnvoyer, sizeof(message), 0, (struct sockaddr*)&adrDist, addrDist);
		printf("Le serveur a envoyé %d octets \n",nbchar);
		if(messageAEnvoyer->fin)
			printf("Le serveur envoi une demande de fermeture de connexion\n");
		if(messageAEnvoyer-> taille > 0)
		{
			enfiler(fenetreEmission, *messageAEnvoyer); //mémorise le datagramme envoyé
			printf("Le datagramme n°%d a été envoyé et sauvegarder dans la fenetre d'émission\n", messageAEnvoyer->seq);
			printf("Fenêtre d'émission : \n");
			afficherFile(fenetreEmission);
		}
		
		PDAE = (PDAE+1)%N;
	}
	printf("Fin envoie première fenêtre\n");
	//nbLuEnvoi = read(input_fd, messageAEnvoyer->buf, BUFFER_LENGTH);

	struct timeval dureeTimeout = T_timeval(timeout);
	//start le timer
	T_init();

	while(!(messageAEnvoyer->fin && messageRecu->fin))
	{

		if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &dureeTimeout, sizeof(dureeTimeout)) < 0) 
		{
		    perror("Error setsockopt");
		}
		else if(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &dureeTimeout, sizeof(dureeTimeout))<= T_get()) //si on est encore dans le timer 
		{
			if(!messageRecu->fin)
			{
				//printf("Coté serveur : Encore des messages a recevoir \n");
				nbLuRecoi = recvfrom(fd, messageRecu, sizeof(message), 0, (struct sockaddr*)&adrLocale, &addrLocale);

				if(messageRecu->seq == DA){
					printf("Coté serveur message recu fin | %d\n", messageRecu->fin);
					printf("Le serveur a recu %d octets\n", nbLuRecoi);
					if(messageRecu->fin)
						printf("Le serveur ferme la connexion l'émetteur (client) a envoyé une demande de fermeture\n");
					if(messageRecu->taille > 0)
						write(output_fd, messageRecu->buf, nbLuRecoi);
					DA++;
				}
				
			}

			if(!messageAEnvoyer->fin)
			{
				//printf("Coté serveur : Encore des messages a envoyer \n");

				// fonction émission
				// on fait de la place dans la fenetre d'émission
				for(i=PAA; i!=messageAEnvoyer->ack; i=(i+1)%N )
				{
					defiler(fenetreEmission);
					PAA++;
				}

				// si la fenêtre est vide, on désarme le timeout
				if(PAA==PDAE) 
					T_stop();

				// envoi datagrammes
				for(i=PDAE; i!=PAA+LARGEUR_FENETRE; i=(i+1)%N)
				{
					nbLuEnvoi = read(input_fd, messageAEnvoyer->buf, BUFFER_LENGTH);
					messageAEnvoyer->taille = nbLuEnvoi;
					messageAEnvoyer->seq = PDAE;
					messageAEnvoyer->ack = DA;
					if(nbLuEnvoi < BUFFER_LENGTH){
						messageAEnvoyer->fin=1;
						printf("Coté serveur plus de message a envoyer | %d\n", messageAEnvoyer->fin);
					}
					nbchar=sendto(fd, messageAEnvoyer, sizeof(message), 0, (struct sockaddr*)&adrDist, addrDist);
					printf("Le serveur a envoyé %d octets \n",nbchar);
					if(messageAEnvoyer->fin)
						printf("Le serveur envoi une demande de fermeture de connexion\n");
					if(messageAEnvoyer-> taille > 0)
					{
						enfiler(fenetreEmission, *messageAEnvoyer); //mémorise le datagramme envoyé
						printf("Le datagramme n°%d a été envoyé et sauvegarder dans la fenetre d'émission\n", messageAEnvoyer->seq);
						printf("Fenêtre d'émission : \n");
						afficherFile(fenetreEmission);
					}
					PDAE = (PDAE+1)%N;
				}
			}
			if(! T_isSet())
			    T_init();
			

		}
		else // expiration timeout
		{
			// réémission de tous les datagrammes non acquittés.
			for(i=PAA; i!=PDAE; i=(i+1)%N)
			{
				message mess = fileGet(fenetreEmission, i);
				messageAEnvoyer = &mess;
				messageAEnvoyer->ack = DA;
				if(nbLuEnvoi < BUFFER_LENGTH){
					messageAEnvoyer->fin=1;
					printf("Coté serveur plus de message a envoyer | %d\n", messageAEnvoyer->fin);
				}
				nbchar=sendto(fd, messageAEnvoyer, sizeof(message), 0, (struct sockaddr*)&adrDist, addrDist);
				printf("Le serveur a envoyé %d octets \n",nbchar);
				if(messageAEnvoyer->fin)
					printf("Le serveur envoi une demande de fermeture de connexion\n");
			}
			T_init();
		}
	}
	close(input_fd);
	close(output_fd);
	return 0;
}
