#include <stdio.h>
#include <stdlib.h>
#include "file.h"

/**
* Initialise la file
* @return : pointeur vers la file qui a été initialisé
*/
File* initialiser(){
    File *file = malloc(sizeof(*file));
    file->premier = NULL;
    return file;
}

/**
* Permet d'ajouter un élément à la file
* @param file : pointeur sur la file
* @param msg : élément à ajouter
*/
void enfiler(File *file, message msg){
    Element *nouveau = malloc(sizeof(*nouveau));
    if (file == NULL || nouveau == NULL)
        exit(EXIT_FAILURE);

    nouveau->msg = msg;
    nouveau->suivant = NULL;

    if (file->premier != NULL){
        // On se positionne à la fin de la file
        Element *elementActuel = file->premier;
        while (elementActuel->suivant != NULL){
            elementActuel = elementActuel->suivant;
        }
        elementActuel->suivant = nouveau;
    }else{ // La file est vide, notre élément est le premier
        file->premier = nouveau;
    }
}

/**
* Permet d'enlever un élément de la file
* @param file : pointeur sur la file
* @return messageDefile : le message defile
*/
message defiler(File *file){
    if (file == NULL)
        exit(EXIT_FAILURE);
   
    message messageDefile;

    // On vérifie s'il y a quelque chose à défiler
    if (file->premier != NULL){
        Element *elementDefile = file->premier;

        messageDefile = elementDefile->msg;
        file->premier = elementDefile->suivant;
        free(elementDefile);
    }

    return messageDefile;
}

/**
*   Renvoie le message mémorisé
*   @param file : file 
*   @param seq : numéro de séquence du datagramme
*   @return message : le datagramme correspondant
*/
message fileGet(File* file, int seq)
{
    int i = 0;
    short int suite = 1;
    Element *elt = file->premier;
    Element *eltATrouver = NULL;

    while(suite)
    {
        //si on a trouvé l'élement correspondant on le renvoie
        if(i == seq)
        {
            eltATrouver = elt->suivant;
        }
        else //sinon on continue de chercher et on incremente
        {
            i++;
        }
    }
    return eltATrouver->msg;
}

/**
* Permet d'afficher une file
* @param file : pointeur sur la file a afficher
*/
void afficherFile(File *file){
    if (file == NULL)
        exit(EXIT_FAILURE);

    Element *element = file->premier;

    while (element != NULL){
        //buff trop long et non affiché
        printf("-> msg.fin : %d, msg.taille : %d, msg.seq : %d, msg.ack %d\n", element->msg.fin, element->msg.taille, element->msg.seq, element->msg.ack);
        element = element->suivant;
    }
    printf("\n");
}


//initialise un message
message* initMessage()
{
    message* m = malloc(sizeof(struct message_s));
    m->fin=0;
    m->taille=0;
    m->seq=0;
    m->ack=0;
    return m;
}


//initialise l'acquitement
ack* initAck()
{
    ack* a = malloc(sizeof(struct ACK));
    a->numAck = 0;
    return a;
}
