
/*======================================================
			client.c
			GoBackN
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
#include "file.h"
#include "timer.h"


int main(int argc, char *argv[])
{
	const int LARGEUR_FENETRE = 3;
	const int N = 4; // 8 + 1 : modulo
	const float timeout = 10.0;

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
	adrLocale.sin_port = htons(0); //si on met le port a 0 bind attribura automatiquement un des ports

	printf("Ip client : %s\n", inet_ntoa(adrLocale.sin_addr));
	printf("Port locale : %d\n", ntohs(adrLocale.sin_port));

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

	socklen_t a = sizeof(adr);
	char buffConnexion[0];
	int nbCharCon = 0;
	nbCharCon = sendto(fd, buffConnexion, 0, 0, (struct sockaddr*)&adr, a);
	printf("Connexion envoie d'un datagramme de %d caractère\n", nbCharCon);

	//Attend le ack 0 > pour etablir la connexion
	int nbCharAck = 0;
	ack* ak = initAck();
	printf("En attente de recevoir un ack pour la connexion\n");
	nbCharAck = recvfrom(fd, ak, 0, 0, (struct sockaddr*)&adr, &a);
	printf("Recu ack(%d) de %d caractères\n", ak->numAck, nbCharAck);
	//si on recoit ce ack on peut passer a la suite vu que c'est le premier ack recu
	
	message* messageRecu = initMessage();
	message* messageAEnvoyer = initMessage();
	int nbLuEnvoi = 0;
	int nbLuRecoi = 0;
	socklen_t addrLocale = sizeof(adrLocale);

	int PDAE = 0; // la borne sup de la fenêtre -1
	int PAA = 0; // Premier acquittement attendu (borne inf de la fenêtre d'émission)
	int DA = 0;
	int nbEltFile = 0; //nombre delement actuel dans la file
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
			printf("Coté client plus de message a envoyer | %d\n", messageAEnvoyer->fin);
		}
		nbchar=sendto(fd, messageAEnvoyer, sizeof(message), 0, (struct sockaddr*)&adr, a);
		printf("Le client a envoyé %d octets \n",nbchar);
		if(messageAEnvoyer->fin)
			printf("Le client envoi une demande de fermeture de connexion\n");
		if(messageAEnvoyer->taille > 0)
			enfiler(fenetreEmission, *messageAEnvoyer); //mémorise le datagramme envoyé
		printf("Le datagramme n°%d a été envoyé et sauvegarder dans la fenetre d'émission\n", messageAEnvoyer->seq);
		printf("Fenêtre d'émission : \n");
		afficherFile(fenetreEmission);
		PDAE = (PDAE+1)%N;
	}
	nbEltFile = PDAE;

	printf("Fin envoie première fenêtre\n");
	//nbLuEnvoi = read(input_fd, messageAEnvoyer->buf, BUFFER_LENGTH);

	struct timeval dureeTimeout = T_timeval(timeout);
	//start le timer
	T_init();

	while(!messageAEnvoyer->fin)
	{

		if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &dureeTimeout, sizeof(dureeTimeout)) < 0) 
		{
		    perror("Error setsockopt");
		}
		else if(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &dureeTimeout, sizeof(dureeTimeout))<= T_get()) //si on est encore dans le timer 
		{

			if(!messageAEnvoyer->fin)
			{
				//printf("Coté client : Encore des messages a envoyer \n");

				//Reception des acquittements
				nbLuRecoi = recvfrom(fd, messageRecu, sizeof(message), 0, (struct sockaddr*)&adrLocale, &addrLocale);
				printf("Recu ack(%d) \n", messageRecu->ack);
				DA++;

				// fonction émission
				// on fait de la place dans la fenetre d'émission
				for(i=PAA; i!=messageRecu->ack; i=(i+1)%N) 
				{
					printf("Defile \n");
					defiler(fenetreEmission);
					nbEltFile--;
					PAA++;
				}
				printf("Nombre d'élément dans la file %d\n", nbEltFile);
				afficherFile(fenetreEmission);

				// si la fenêtre est vide, on désarme le timeout
				if(PAA==PDAE){
					printf("Timerstop\n");
					T_stop();
				}

				printf("i %d\n", i);
				printf("PAA : %d\n", PAA);
				printf("PDAE : %d\n", PDAE);
				// envoi datagrammes
				for(i=PDAE; i!=PAA+LARGEUR_FENETRE; i=(i+1)%N)
				{
					printf("Envoie datagrammes ...\n");
					printf("i %d / PDAE %d / PAA + LARGEUR_FENETRE %d\n", i, PDAE, PAA+LARGEUR_FENETRE);
					nbLuEnvoi = read(input_fd, messageAEnvoyer->buf, BUFFER_LENGTH);
					messageAEnvoyer->taille = nbLuEnvoi;
					messageAEnvoyer->seq = PDAE;
					messageAEnvoyer->ack = DA;
					if(nbLuEnvoi < BUFFER_LENGTH){
						messageAEnvoyer->fin=1;
						printf("Coté client plus de message a envoyer | %d\n", messageAEnvoyer->fin);

					}
					printf("messageAEnvoyer->seq  %d | ack %d\n", messageAEnvoyer->seq, messageRecu->ack);
					if(messageAEnvoyer->seq >= messageRecu->ack)
					{
						nbchar=sendto(fd, messageAEnvoyer, sizeof(message), 0, (struct sockaddr*)&adr, a);
						printf("Le client a envoyé %d octets \n",nbchar);
					}
					if(messageAEnvoyer->fin)
						printf("Le client envoi une demande de fermeture de connexion\n");

					if(messageAEnvoyer->taille > 0 && nbEltFile < LARGEUR_FENETRE) //on enfile les datagrammes normaux si on a de la place dans notre file
					{
						enfiler(fenetreEmission, *messageAEnvoyer); //mémorise le datagramme envoyé
						printf("Le datagramme n°%d a été envoyé et sauvegarder dans la fenetre d'émission\n", messageAEnvoyer->seq);
						printf("Fenêtre d'émission : \n");
						afficherFile(fenetreEmission);
						PDAE = (PDAE+1)%N;
						printf("PDAE ??? %d\n", PDAE);
					}
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
				printf("Réémission de tous les datagrammes non acquittés \n");

				message mess = fileGet(fenetreEmission, i);
				messageAEnvoyer = &mess;
				messageAEnvoyer->ack = DA;
				if(nbLuEnvoi < BUFFER_LENGTH){
					messageAEnvoyer->fin=1;
					printf("Coté client plus de message a envoyer | %d\n", messageAEnvoyer->fin);
				}
				nbchar=sendto(fd, messageAEnvoyer, sizeof(message), 0, (struct sockaddr*)&adr, a);
				printf("Le client a envoyé %d octets \n",nbchar);
				if(messageAEnvoyer->fin)
					printf("Le client envoi une demande de fermeture de connexion\n");
			}
			printf("Timeout ! \n");
			T_init();
		}

	}	
	close(input_fd);
	close(output_fd);
	return 0;
}
