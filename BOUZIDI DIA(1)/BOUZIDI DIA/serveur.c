/**
 * \file serveur.c
 * \brief Serveur de calcul de sommes préfixées.
 * \author Louisa BOUZIDI et Modou Ndiar DIA
 * \version 0.1
 * \date 28 decembre 2022
 *
 * Ce programme effectue le traitement suivant: Il crée un tube (s’il n’existe pas)
 * dans le dossier où il se trouve puis entre dans une boucle infinie dans
 * laquelle il:
 *   ---> lit une requête depuis le tube
 *   ---> se clone et crée (grâce à l’appel fork) un processus fils (nommé worker)
 *   ---> Le processus fils (worker ==0) fait appel à une fonction “traitementWorker”
 *        à laquelle il fournit le PID du client qui a envoyé la requête, la taille du
 *        tableau de données déjà mis en mémoire partagée et l’opération à appliquer
 *        sur les données.
 *   ---> la fonction “traitementWorker” applique l'algorithme de Hills Steel Scan
 *        et crée crée plusieurs threads pour le calcul parallèle.
 *   ---> le serveur (le père) attend la fin de l’exécution du worker (son fils) pour
 *        reboucler et traiter une nouvelle requête d'un client
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include "conf.h"

void creerTube();
int traitementWorker(int pid, int dataSize, int operation);

int main(int argc, char *argv[]) {

    creerTube();  // Création du tube

    // Récupération du descripteur du tube en lecture
    // FIFO_NAME contient le nom du tube partagé entre le client et le serveur
    // On suppose que le client et le serveur sont dans le même répertoire
    // *********************************************************************

    int fdread = 0;
    if ((fdread = open(FIFO_NAME, O_RDONLY)) == -1) {
        fprintf(stderr, "Impossible d'ouvrir le tube en lecture: %s\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    // boucle infinie de lecture des requêtes depuis le tube
    // ***************************************************
    struct requete req;
    int numRequette = 0;
    while (1) {
        while (1) {
            if (read(fdread, &req, sizeof(req)) > 0) break;
        }
        numRequette++;
        printf("\n\nRequête courante : ");
        printf("(Pid=%d, Taille=%d, OP=%d)\n", req.pid, req.dataSize, req.operation);

        pid_t worker= fork();

        if(worker==0) {
            traitementWorker(req.pid, req.dataSize, req.operation);
            exit(EXIT_SUCCESS);
        }

        int	status;
        while (numRequette==10){
            waitpid(worker, &status, 0); // Attendre le fils (worker)
            printf("Serveur : Mon worker PID=%d s'est terminé avec success (statut: %d)\n", worker, status);
        }

    }
    return 0;
}

/**********************************************************************/
/* Fonction permettant de créer un tube nommé qui sera utilisé        */
/* par des processus clients en écriture et par le serveur en lecture */
/**********************************************************************/
void creerTube(void) {
    // On supprime le tube s'il existe déjà
    if (mkfifo(FIFO_NAME, 0666) == -1) {
		if (EEXIST== errno){
			printf("\nSuppression du tube existant: %s\n", FIFO_NAME);
			unlink(FIFO_NAME);
			creerTube();
		}
		else {
			printf("\nErreur de création du tube: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
    }
    else {
        printf("\nCréation du tube: %s\n", FIFO_NAME);
    }
} //----------------------------------------------------------------------

int traitementWorker(int pid, int dataSize, int operation) {

    // Etape1 : Obtenir l'id du segment de mémoire partagé en appelant l'appel système
    // shmget() en lui fournissant le PIP du client comme clé
    int shmid;
    struct shmseg *shmp;
    shmid = shmget(pid, sizeof(struct shmseg), 0644);
    if (shmid == -1) {
        perror("Shared memory");
        return 1;
    }

    // Etape2 : Attacher le segment de mémoire partagée pour avoir un pointeur vers sa zone.
    // *************************************************************************************
    shmp = shmat(shmid, NULL, 0);
    if (shmp == (void*) -1) {
        perror("Shared memory attach");
        return 1;
    }

    // Etape3 : Faire les calculs sur les données du client et lui rendre le résultat
    // ******************************************************************************

    int * data = &shmp->data[0];
    int data_new[NB_MAX_VALEURS];       // tableau temporairement servant pour les calculs
    pthread_t Threads[NB_MAXI_THREADS];

    typedef struct hills_t hills_t;

    struct hills_t {
        int etape;
        int indice;
    };

    void afficherTableau(int *T, int size) {
        printf("[");
        for (int i=0; i<size; i++) {
            printf("%d", T[i]);
            if (i!=size-1) printf(", ");
        }
        printf("]\n");
    }//-------------------------------------

    int pgcd(int a, int b) {
        while (a!=b) {
            if (a>b) {
                a = a-b;
            } else {
                b = b-a;
            }
        }
        return a;
    }//-------------------------------------

    /* Fonction de claul selon l'agorithme de Hills Steel Scan
      qui est exécutée par chaque thread pour effectuer une
      calcul sur un élément d'indice i et un autre d'indice
      égale à  i - 2 à la puissance l'étape
      ************************************************************/
    void *calcul(void*arg) {

        hills_t *hills = (hills_t *)arg;
        int i = hills->indice;
        int e = hills->etape;
        int i1 = (int)(i-pow(2, e));

        switch (operation) {
        case ADDITION: {
            data_new[i] = data[i] + data[i1];
            break;
        }
        case SOUSTRACTION: {
            data_new[i] = data[i] - data[i1];
            break;
        }
        case MULTIPLICATION: {
            data_new[i] = data[i] * data[i1];
            break;
        }
        case MAXIMUM: {
            data_new[i] = (data[i] > data[i1]) ? data[i] : data[i1];
            break;
        }
        case MINIMUM: {
            data_new[i] = (data[i] < data[i1]) ? data[i] : data[i1];
            break;
        }
        case PGCD: {
            data_new[i] = pgcd(data[i], data[i1]);
            break;
        }
        }

        printf("\n  --> indice=%d data[%d]=data[%d] + data[%d]=%d",i,i,i,i1,data_new[i]);
        printf("= %d + %d ", data[i], data[i1]);
        free(hills);
        return NULL;
    } //--------------------------------------------------------------------------------

    // Récupèration des données depuis la mémoire partagée
    for (int i=0; i<dataSize; i++) {
        data_new[i] = shmp->data[i];
    }

    printf("\nDonnées de départ : ");
    afficherTableau(data, dataSize);

    // calcul  du nombre maximum d'étapes de l'agorithme de hills steel scan
    unsigned int nbEtapes;
    nbEtapes = log2(dataSize);
    if (pow(2,nbEtapes) <dataSize) {
        nbEtapes=nbEtapes+1;
    }
    printf ("\nNombre de valeurs de données = %d et nombre d'étapes = %d\n", dataSize, nbEtapes);

    // boucle de calcul selon l'algorithme de Hills Steel Scan
    for (int etape=0; etape<=nbEtapes; etape++) {
        sleep(1);
        printf("\nEtape %d  ========================================", etape);

        // pour chaque étape on parcour le tableau des données en commençant
        // par un indique égale à 2 puissance l'étape
        for (int i=(int)pow(2,etape); i<dataSize; i++ ) {
            hills_t *hills  = malloc(sizeof(hills_t));
            hills->etape   = etape;
            hills->indice  = i;
            pthread_create(&Threads[i], NULL, calcul, hills);
        }

        // j'attends la fin de l'étape pour commencer l'étape suivante
        for (int i=pow(2, etape); i<dataSize; i++ ) {
            pthread_join(Threads[i], NULL);
        }

        // on met les données qui viennent d'être cacluées dans le
        // tableau data de sorte à ce qu'elle soit prises en compte
        // dans étape suivante
        for (int k=0; k<dataSize; k++) {
            data[k] = data_new[k];
        }

        printf("\n\nCaclul intermédiaire : ");
        afficherTableau(data_new, dataSize);
    }

    printf("\n\nRésultats final \n");
    printf("******************************************************\n");
    afficherTableau(data, dataSize);
    printf("******************************************************\n");

    //***********************************************************************************************************
    // on indique au client que les calculs sont terminés et que le résultat
    // est disponible en mémoire partagée en mettant le champ complete à 2
    // ********************************************************************
    shmp->status = FIN_REMISE_RESULTATS;

    // Détacher le segment de mémoire partagé et on reboucle pour attendre une autre requête
    // ***********************************************************************************

    if (shmdt(shmp) == -1) {
        perror("shmdt");
        return 1;
    }
    return 0;
}



