#ifndef H_FILE
#define H_FILE

#define BUFFER_LENGTH 1024

typedef struct message_s 
{
	char buf[BUFFER_LENGTH];
	short int fin;
	int taille;
	int seq;
	int ack;
}message;

typedef struct ACK
{
	int numAck; //numéro du message a envoyer
}ack;

typedef struct Element Element;
struct Element
{
    message msg;
    Element *suivant;
};

typedef struct File File;
struct File
{
    Element *premier;
};

File* initialiser(void);
void enfiler(File *file, message msg);
message defiler(File *file);
void afficherFile(File *file);
message fileGet(File *file, int seq);

ack* initAck(void);
message* initMessage(void);

#endif
