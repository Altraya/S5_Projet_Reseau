
/*======================================================
			serveur.c
	Transfert de fichiers bidirectionnelle
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

int main(int argc, char **argv)
{
	if (argc < 4){
		fprintf(stderr, "Usage: %s <fichier_a_envoyer>  <fichier_recu> <port_local>\n", argv[0]);
		exit(1);
	};
	char* nomFichierAEnvoyer = argv[1];
	char* nomFichierRecu = argv[2];
	printf("Nom fichier a envoye: %s\n", nomFichierAEnvoyer);
	printf("Nom fichier recu %s\n", nomFichierRecu);
	int port = atoi(argv[3]);
	printf("Port : %d\n", port);

	int input_fd;
	int output_fd;
	int nbchar;
	int connexion = 0;

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
	else
	{
		if(bind(fd,(struct sockaddr*)&adrLocale,sizeof(adrLocale))==-1)
		{
			perror("bind");
			exit(2);
		}
		else
		{		
			struct sockaddr_in adrDist;
			if(!connexion)
			{
				//Connexion
				//struct sockaddr_in adrDist;
				adrDist.sin_family = AF_INET;
				adrLocale.sin_addr.s_addr = htonl(INADDR_ANY);
				adrDist.sin_port = htons(port);
			}
			else
			{

				// ouvrir le fichier a envoyer en lecture
				input_fd = open(nomFichierAEnvoyer, O_RDONLY);
				if(input_fd<0)
				{	
					perror("open input"); 
					exit(1); 		
				}

				// ouvrir le fichier en ecriture que l'on recoit
				output_fd = open(nomFichierRecu, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR);
				if(output_fd<0)
				{	
					perror("open output"); 
					exit(1); 		
				}

				// copie :
				char bufferEnvoi[BUFFER_LENGTH];
				int nbLuEnvoi = 0;
				int finished1 = 0;
				char bufferRecu[BUFFER_LENGTH];
				int nbLuRecoi = 0;
				int finished2 = 0;
				while(!(finished1 && finished2))
				{
					nbLuEnvoi = read(input_fd, bufferEnvoi, BUFFER_LENGTH);
					if(nbLuEnvoi < BUFFER_LENGTH)
						finished1 = 1;
					socklen_t a =sizeof(adrDist); 
					nbchar=sendto(fd, bufferEnvoi, nbLuEnvoi, 0, (struct sockaddr*)&adrDist,a);
					printf("Nombre de caractère envoyé : %d\n",nbchar);

					socklen_t b =sizeof(adrLocale); 
					nbLuRecoi = recvfrom(fd,bufferRecu,1024,0,(struct sockaddr*)&adrLocale,&b);
					printf("Le client a recu : %d octets\n", nbLuRecoi);
					if(nbLuRecoi < BUFFER_LENGTH)
						finished2 = 1;
					write(output_fd, bufferRecu, nbLuRecoi);
				}
			}
		}
		close(input_fd);
		close(output_fd);
		return 0;
	}
}
