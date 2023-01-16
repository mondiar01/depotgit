/**
 *  Created on: 23 déc. 2022
 *  Author: Bouzidi Louisa et Dia Modou Ndiar
 *  Présentation :
 *  	Ce programme implémente le traitement d'un un processus client
 *  	qui demande des calculs (opération de calcul associative) à un
 *  	processus serveur via des requêtes trasmises dans un tube nommé.
 *  	Les données requises par les calculs demandés sont trasmises
 *  	via un segment de mémoire partagée. Ce dernier est crée par le
 *  	client.
 *  	Dès que le caclul est rendu dans le segment de mémoire partagée,
 *  	par le processus serveur, le processus client l'affiche et
 *  	s'arrête.
 *  	La requête est constituée:
 *  	    ---> du pid (id du processus) du processus client
 *  	    ---> du numéro de l'opération de calcul demandée
 *  	    ---> de la taille du tableau de données
 *  	Le ségment de mémoire partagé est composé d'un tableau de valeurs
 *  	(des réels)
 *
 *  	A noter que la clé qui servira pour créer le segment de mémoire
 *  	partagée est le pid du client. C'est pour cette raison que ce pid
 *  	est trasmis au seveur afin que ce dernier puisse accepter au
 *  	segment de mémoire partagée
 *
 *  	On prévoi deux manière pour indiquer la source de données:
 *  	    ---> en ligne de dommande (indication du fichier de données)
 *  	    ---> via le fichier de configuration
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "conf.h"

int  lireData(int *data, int *nbValeurs, char *fichier);
void afficherData(int *tab, int size);
void afficherErreurUsage();
int  dataFileNotExiste(char *f);
void afficherOperationsPossibles();
void afficherErreurOperation();

/* ************************************************************************************/
/*                          Programme principale du client                            */
/* ************************************************************************************/
int main(int argc, char *argv[]) {

    system("clear");

    printf("==> PID du client : %d\n", getpid());

    // Etape 1 : Vérification et récupération des arguments fournis en ligne de commande
    // -----------------------------------------------------------------

    char *fichier = NULL;     // pDataFile : pointeur vers le fichier de données

    if (argc != 3) {
        afficherErreurUsage();  // si l'utilisateur ne donne pas le nom du fichier et le
        return EXIT_FAILURE;    // numéro de l'opération, on lui affiche une erreur d'usage
                                //  ./client <nomFichierDeDonnées> <numéro opération>
    }                           // et on quite le programme

    fichier = argv[1];          // on récupère le nom du fichier de données
                                // fourni en ligne de commande

    int operation;              // opération de calcul à réaliser par les workers
    operation = atoi(argv[2]);  // On récupère le numéro de l'opération à effectuer sur
                                // les données

    if (operation <1 || operation>NB_OPERATIONS) {  // on vérifier le numéro de l'opération
                                                    // Indiquée en ligne de commande
        afficherErreurOperation();                  // si non prévue on affiche un message
        return EXIT_FAILURE;                        // d'erreur et on quite le programme
    }

    // ---------------------------------------------------------------------
    // Etape 2 : Récupération des données depuis le fichier dont le nom est
    // fourni en ligne de commande ce fichier comporte un tableau de valeurs
    // ---------------------------------------------------------------------

    int data[NB_MAX_VALEURS];   // tableau de données
    int nbDataValues = 0;       // nombre d'éléments dans le tableau de données

    if(lireData(data, &nbDataValues, fichier)==EXIT_FAILURE) {
        printf("Erreur dans le fichier de données");
        return EXIT_FAILURE;
    }

    // On affiche les données récupérées
    printf("\n==> %d valeurs lues à partir du fichier %s :\n\n    ", nbDataValues, fichier);
    afficherData(data, nbDataValues);

    // --------------------------------------------------
    // Etape 3 : Création d'un segment de mémoire partagé
    // --------------------------------------------------

    int shmid;              // id du segment de mémoire partagée
    struct shmseg *shmp;    // pointeur vers le segment de mémoire partagé

    /* Obtention de l'identifiant du segment de mémoire partagée
       grâce la l'appel de "shmget" à laquelle il faut indiquer une clé unique
       ici on se servira du pid du processus on indiquera la taille de la zone
       à obtenir et les droits d'accès à cette zone.
       le paramètre IPC_CREAT est pour créer un nouveau segment.*/

    shmid = shmget((int)getpid(), sizeof(struct shmseg), 0644 | IPC_CREAT);

    if (shmid == -1) {
        perror("Shared memory");
        return 1;
    }

    /* Attacher le segment de mémoire partagé à l'espace d'adressage du processus
       en cours afin que ce dernier puisse y accéder.
       L'appel à la fonction "shmat" (shared mémory attach) va renvoyer un pointeur
       vers le segment de mémoire partagée. "shmat exige de lui fournir la clé du
       segment de mémoire partagé renvoyé par la fonction shmget */

    shmp = shmat(shmid, NULL, 0);
    if (shmp == (void*) -1) {
        perror("Shared memory attach");
        return 1;
    }

    // -------------------------------------------------------------------
    // Etape 4 : Transfert des données vers le segment de mémoire partagée
    // -------------------------------------------------------------------

    for (int i = 0; i < nbDataValues; i++) {
        shmp->data[i] = data[i];
    }

    printf("\n==> %d valeurs ont été écrites en mémoire partagée\n", nbDataValues);

    shmp->status = FIN_DEPOT_DATA;

    // ---------------------------------------------------------------------
    // Etape 5 : Création d'une requête. On met 3 champs : le PID du client,
    // la taille du tableau de données et l'opération qui doit être appliqué
    // par les workers sur les données
    // ---------------------------------------------------------------------

    struct requete req;
    req.dataSize  = nbDataValues;
    req.operation = operation;
    req.pid       = getpid();

    // ------------------------------------------------------------
    // Etape 6 : Tentative d'ouverture en écriture du tube de
    // communication partagé avec le seveur afin de lui transmettre
    // une requête. Si echec --> message d'erreur et quitter
    // ------------------------------------------------------------

    int fdwrite;
    if ((fdwrite = open(FIFO_NAME, O_WRONLY)) == -1) {
        printf("\n\nImpossible d'ouvrir le tube en écriture: %s\n",
               strerror(errno));
        exit(EXIT_FAILURE);
    }

    // ---------------------------------------------
    // Etape 7 : Ecriture de la requete dans le tube
    // ---------------------------------------------

    if (write(fdwrite, &req, sizeof(req))>0) {
        printf("\n==> La requête du client est écrite avec success dans le tube\n ");
    }

    // ----------------------------------------------------------
    // Etape 8 : Boucle d'attente des la fin du calcul du côté du
    // serveur et de la remise du résultat par les workers
    // ----------------------------------------------------------

    printf("\n==> Attente de la remise du résultat par les workers (serveur)...\n");
    while (shmp->status == FIN_DEPOT_DATA) {
        continue;
    }

    // -------------------------------
    // Etape 9 : Affichage du résultat
    // -------------------------------

    printf("\n==> Traitelent du coté serveur terminé. Voici le résultat:\n\n    ");
    afficherData(shmp->data, nbDataValues);
    printf("\n");

    // ---------------------------------------------------
    // Etape 10 : Détacher le segment de mémoire partagée
    // ---------------------------------------------------

    if (shmdt(shmp) == -1) {
        perror("shmdt");
        return 1;
    }

    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        perror("shmctl");
        return 1;
    }

    return 0;
}
/*                                Fin du programme principal                          */
/* ************************************************************************************/

/* Implémentation de quelques fonctions utiles */
/***********************************************/

// lecture des données depuis un fichier et renvoi du nombre de ces
// données et de leur valeurs dans un tableau d'entiers
int lireData(int *data, int *nbValeurs, char *fichier) {
    FILE *f;
    char chaine[100];

    f = fopen(fichier, "r");
    if (f == NULL) {
        printf("le fichier de données n'a pas pu être ouvert ...");
        return EXIT_FAILURE;
    }
    char *ptr;
    int i = 0;
    int j = 0;
    while (1) {
        for (int i=0; i<100; i++) chaine[i]='\0';
        char c= '0';
        i = 0;
        while (c!=EOF && c!=' ') {
            c = fgetc(f);
            chaine[i] = c;
            i++;
        }
        data[j] = strtol(chaine, &ptr, 10);
        j++;
        if (c==EOF) break;
    }
    *nbValeurs = j;

    fclose(f);
    return EXIT_SUCCESS;
}

// quelques fonctions d'affichage
// *******************************

void afficherData(int *T, int size) {
    printf("[");
    for (int i=0; i<size; i++) {
        printf("%d", T[i]);
        if (i!=size-1) printf(", ");
    }
    printf("]\n");

}

void afficherErreurUsage() {
    printf("\nNombre d'arguments incorrecte! ");
    printf("vous devez indiquer le nom du fichier de données\n");
    printf("suivi du numéro de l'opération\n");
    afficherOperationsPossibles();
    printf("\n\nVous pouvez faire un test la commande suivante: ./client data 1\n");
    printf("   ---> ./client est le fichier exécutale \n");
    printf("   ---> data est le nom du fichier de données \n");
    printf("   ---> 1 est le numéro de l'opération à appliquer sur les données \n");
    printf("Remarque: vous devez indiquer le chemin complet vers le fichier ");
    printf("Si ce dernier n'est pas dans le même dossier que le fichier exécutable './client'\n\n");
}

void afficherOperationsPossibles() {
    printf("   ---> 1 : ADDITION\n");
    printf("   ---> 2 : SOUSTRACTION\n");
    printf("   ---> 3 : MULTIPLICATION\n");
    printf("   ---> 4 : MAXIMUM\n");
    printf("   ---> 5 : MINIMUM\n");
    printf("   ---> 6 : PGCD\n");
}

void afficherErreurOperation() {
    printf("\n\nLe numéro de l'opération est incorrecte");
    afficherOperationsPossibles();
}
