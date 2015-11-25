
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
	char* nomFichierAEnvoye = argv[1];
	char* nomFichierRecu = argv[2];
	printf("Nom fichier a envoye: %s\n", nomFichierAEnvoye);
	printf("Nom fichier recu %s\n", nomFichierRecu);
	int port = atoi(argv[3]);
	printf("Port : %d\n", port);
	int output_fd;
	struct sockaddr_in adr;
	adr.sin_family = AF_INET;
	adr.sin_addr.s_addr = htonl(INADDR_ANY);
	adr.sin_port = htons(port); 
	int fd;
	int nb_lu = 0;
	fd=socket(AF_INET,SOCK_DGRAM,0);
	
	if(fd==-1)
	{
		perror("socket");
		exit(1);
	}
	else
	{
		if(bind(fd,(struct sockaddr*)&adr,sizeof(adr))==-1)
		{
			perror("bind");
			exit(2);
		}
		else
		{		
			// ouvrir le fichier en ecriture
			output_fd = open(nomFichierRecu, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR);
			if(output_fd<0)
			{	
				perror("open output"); 
				exit(1); 		
			}

			// copie :
			char buffer[BUFFER_LENGTH];
			int finished=0;
			while(!finished)
			{
				socklen_t a =sizeof(adr); 
				nb_lu = recvfrom(fd,buffer,1024,0,(struct sockaddr*)&adr,&a);
				printf("Recu : %d octets\n", nb_lu);
				if(nb_lu < BUFFER_LENGTH)
					finished = 1;
				write(output_fd, buffer, nb_lu);
			}
		}
		close(output_fd);
		return 0;
	}
}
