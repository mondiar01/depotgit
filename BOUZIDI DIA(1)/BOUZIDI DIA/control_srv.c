/**
 ============================================================================
 Name        : Projet_SE.c
 Author      : Bouzidi Louisa et Dia Modou Ndiar
 Version     : 1.0
 Description : Application client/serveur permettant à des processus clients
 	 	 	   de demander des traitements à un serveur à travers des requêtes
 	 	 	   envoyées via un tube nommé. Les données requises pour les
 	 	 	   traitement demandés par les clients sont transmises via
 	 	 	   un segment de mémoire partagée.

 	 	 	   Ces traitements consiste à appliquer une opération associative
 	 	 	   sur un tableau de valeurs de façon récurence de sorte à ce que
 	 	 	   cette opération soit appliquée au niveau "i" à toutes valeurs
 	 	 	   précédentes (i allant de 0 à i-1)

 	 	 	   Par ailleurs, ces traitement seront réalisés selon un traitement
 	 	 	   parallèles des workers (plusieurs thread) crés par le processus
 	 	 	   serveur. En somme, on peut considérer que nous avons à faire un
 	 	 	   petit serveur de traitements.

 Solution	:  Cette application est composée des éléments suivants:
 	 	 	   ---> un programme nommé "client.c" qui implémente le traitement
 	 	 	        des clients
 	 	 	   ---> un programme nommé "serveur.c" qui implémente le traitement
 	 	 	        du serveur
 	 	 	   ---> une bibliothèque nommée "utils" qui contient quelques
 	 	 	        fonctions utilitaires :
 	 	 	        => Lecture des données depuis un fchiers (utilisée par les
 	 	 	           processus clients
 	 	 	        => Afficher un tableau de données
 	 	 	        => ...
 	 	 	   ---> un fichier makefile ....
 	 	 	   ---> un fichier de données (texte) constituées de valeurs entières
 	 	 	    	ou réelles sparées par des espaces
 	 	 	   ---> un fichier de configuration de l'application
 	 	 	        => Emplacement des données
 	 	 	        => Dimensionnement des tableaux de données
 	 	 	        => Opérations associatives à réaliser

 Ce programme constitut le centre de contrôle des serveur et des clients et défini
 le paramétrage inital de notre application et offre les traitement suivants:
 ---> Paramétrer l'application (création du fichier de configuration et
 	  fixation des paramètres (data sorce, taille du tubes, etc...)
 ---> Lancement du serveur (démon)
 ---> Arrêt du serveur (démon)
 ---> Création d'une requêtte et lancement d'un client
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include "conf.h"

int menu(void);

int main(void) {
    pid_t p_serveur = -10;
    while (1) {
        int choix = menu();
        system("clear");

        switch (choix) {
        case 1 :
            system("clear");
            if (p_serveur == -10) {
                p_serveur = fork();
                if (p_serveur==0) {
                    execl ("./serveur", "", NULL);
                }
                printf("Serveur lancé sous le PID : %d\n", p_serveur);
            } else {
                printf("Serveur déjà en exécution ...");
            }
            break;
        case 2 :
            system("clear");
            if (p_serveur!=-10) {
                kill(p_serveur, SIGKILL);
                printf("Serveur correctement stoppé !");
                p_serveur = -10;
            } else {
                printf("Serveur déjà stoppé, donc rien à faire !");
            }
            break;
        default: break;
        }

        if (choix==3)
        {
            system("clear");
            if (p_serveur!=-10) {
                kill(p_serveur, SIGKILL);
                printf("Serveur correctement stoppé !");
                p_serveur = -10;
            } else {
                printf("Serveur déjà stoppé, donc rien à faire !");
            }
            printf("\nApplication terminée correctement \n");
            break;

        }

        printf("\n ----> Tapez une touche pour revenir au menu principal...");
        getchar();
        getchar();
    }
    return EXIT_SUCCESS;
}

/*****************************************************************/
/* Fonction affichant le menu principal du programme de contrôle */
/* des serveurs et des clients                                   */
/*****************************************************************/
int menu(void) {
    int choix=0;
    while (1) {
        system("clear");
        printf("\n");
        puts("!!! Bienvenue dans notre serveur de calculs !!!\n");
        printf(" │***************************************│\n");
        printf(" │ --> 1 - Lancer le serveur de calculs  │\n");
        printf(" │ --> 2 - Arrêter le serveur de calculs │\n");
        printf(" │ --> 3 - Quitter                       │\n");
        printf(" │***************************************│\n");
        printf("\n");

        choix = getchar();
        if ((choix >'0') & (choix <'4')) {
            break;
        }
    }
    printf("\nVotre choix %d\n", (int)choix-48);
    return (int)choix-48;
}

