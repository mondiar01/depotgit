/**
 * conf.h
 *
 *  Created on: 23 déc. 2022
 *      Author: Bouzidi Louisa et Dia Modou Ndiar
 */

#ifndef CONF_H_
#define CONF_H_

#define TRUE  1
#define FALSE 0

#define FIFO_NAME "./tube_fifo" // Nom du tube
#define BUFFER_LENGTH 30        // Longeur du buffer de lecture
#define DATA_PATH "./data"		// nom du fichier de données par défaut
#define BUF_SIZE 256            // taille maxi du buffer de données
#define NB_MAX_VALEURS 256      // Nombre maximum de valeurs dans le tableau de données
#define NB_MAXI_THREADS 256     // Nombre maximum de threadhs
#define NB_MAX_WORKERS  200     // Nombre MAXIMUM

// Défintion des constantes permettant d'identifier les opérations de calcul
// *************************************************************************

#define NB_OPERATIONS     7

#define ADDITION          1
#define SOUSTRACTION      2
#define MULTIPLICATION    3
#define MAXIMUM           4
#define MINIMUM           5
#define PGCD              6



// Défintion des constantes permettant d'identifier qui
// occupe le segment de mémoire partagé à un moment donnée
// *******************************************************
#define DEBUT_DEPOT_DATA     0
#define FIN_DEPOT_DATA       1
#define FIN_REMISE_RESULTATS 2


    // création d'une variable "shmseg" est une struture composée de 2 champs
    // -> data : un tableau de données
    // -> status : pour synchroniser le client et le serveur
    //    status = DATA_FOURNIES (1) indique que les données sont déposé en mémoire
    //             partagée par le client
    //    status = CALCUL_TERMINE (2) indique que le worker a rendu le résultats
    //             dans la mémoire  partagé


struct shmseg {
    int data[NB_MAX_VALEURS];
    int status;
};


/*****************************************************************/
/* Structure "requete" permettant de définir le type des données */
/* Elle est composée de 3 champs:                                */
/*   ---> Le PID du client                                       */
/*   ---> La taille du tableau de données                        */
/*   ---> Le numéro de l'opération à effectuer par les workres   */
/*          1 : addition                                         */
/*          2 : soustraction                                     */
/*          3 : multiplication                                   */
/*          4 : maximum                                          */
/*          5 : minimum                                          */
/*          6 : PGCD                                             */
/*****************************************************************/

struct requete {
    int pid;
    int dataSize;
    int operation;
};



int listWorkers [NB_MAX_WORKERS];
int nbWorkers;


#endif /* CONF_H_ */
