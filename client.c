/*======================================================
			client.c
	Transfert de fichiers à sens unique
./client <fichier_a_envoyer> <adr_IP_dist> <port_dist>
./client coucou.txt 192.168.132.128 5000
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


int main(int argc, char *argv[])
{
	int input_fd;
	int nbchar;
	if (argc < 4){
		fprintf(stderr, "Usage: %s <fichier_a_envoyer> <adr_IP_dist> <port_dist>\n", argv[0]);
		exit(1);
	};
	char* nomFichier = argv[1];
	char* adresseIp = argv[2];
	int port = atoi(argv[3]);
	{
		struct sockaddr_in adr;
		adr.sin_family = AF_INET;
		inet_pton(AF_INET,adresseIp,&(adr.sin_addr));
		adr.sin_port = htons(port); 
		int fd;
		fd=socket(AF_INET,SOCK_DGRAM,0);
		if(fd==-1)
		{
			perror("socket");
			exit(1);
		}
		else
		{
			// ouvrir le fichier en lecture
			input_fd = open(nomFichier, O_RDONLY);
			if(input_fd<0)
			{	
				perror("open input"); 
				exit(1); 		
			}

			char buffer[BUFFER_LENGTH];

			int finished=0;
			while(!finished)
			{
				int nb_lu = read(input_fd, buffer, BUFFER_LENGTH);
				if(nb_lu < BUFFER_LENGTH)
					finished = 1;
				socklen_t a =sizeof(adr); 
				nbchar=sendto(fd, buffer,nb_lu,0, (struct sockaddr*)&adr,a);
				printf("Nombre de caractère envoyé : %d\n",nbchar);
			}
		}
		close(input_fd);
		return 0;
	}
}
