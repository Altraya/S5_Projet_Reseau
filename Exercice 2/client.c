
/*======================================================
			client.c
	Transfert de fichiers bidirectionnelle
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


int main(int argc, char **argv)
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
	int portLocal = atoi(argv[5]);
	
	//Adresse distante
	struct sockaddr_in adr;
	adr.sin_family = AF_INET;
	inet_pton(AF_INET,adresseIp,&(adr.sin_addr));
	adr.sin_port = htons(port);

	//Adresse locale
	struct sockaddr_in adrLocale;
	adrLocale.sin_family = AF_INET;
	adrLocale.sin_addr.s_addr = htonl(INADDR_ANY);
	//inet_pton(AF_INET, "127.0.0.1", &(adr.sin_addr));
	adrLocale.sin_port = htons(portLocal);

	int fd;
	fd=socket(AF_INET,SOCK_DGRAM,0);
	if(fd==-1)
	{
		perror("socket");
		exit(1);
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

		socklen_t a = sizeof(adr);
		int connexion = 0;
		if(!connexion)
		{
			char buffConnexion[0];
			int nbCharCon = 0;
			nbCharCon = sendto(fd, buffConnexion, 0, 0, (struct sockaddr*)&adr, a);
			printf("Connexion envoie d'un datagramme de %d caractère\n", nbCharCon);
			connexion = 1;
		}
		else
		{
			char bufferEnvoi[BUFFER_LENGTH];
			int nbLuEnvoi = 0;
			int finished1 = 0;
			char bufferRecu[BUFFER_LENGTH];
			int nbLuRecoi = 0;
			int finished2 = 0;
			while(!(finished1 && finished2) && connexion)
			{
				nbLuEnvoi = read(input_fd, bufferEnvoi, BUFFER_LENGTH);
				if(nbLuEnvoi < BUFFER_LENGTH)
					finished1 = 1;
				
				nbchar=sendto(fd, bufferEnvoi, nbLuEnvoi, 0, (struct sockaddr*)&adr, a);
				printf("Nombre de caractère envoyé : %d\n",nbchar);

				socklen_t b = sizeof(adrLocale); 
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

