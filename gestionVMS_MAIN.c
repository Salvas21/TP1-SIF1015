//#########################################################
//#
//# Titre : 	UTILITAIRES (MAIN) TP1 LINUX Automne 21
//#				SIF-1015 - Système d'exploitation
//#				Université du Québec à Trois-Rivières
//#
//# Auteur : 	Francois Meunier
//#	Date :		Septembre 2021
//#
//# Langage : 	ANSI C on LINUX 
//#
//#######################################

#include "gestionListeChaineeVMS.h"
#include "gestionVMS.h"

//Pointeur de tête de liste
struct noeud* head;
//Pointeur de queue de liste pour ajout rapide
struct noeud* queue;
// nombre de VM actives
int nbVM;

pthread_mutex_t mutexH;

pthread_mutex_t mutexQ;


int main(int argc, char* argv[]){

	//Initialisation des pointeurs
	head = NULL;
	queue = NULL;
	nbVM = 0;

	if (pthread_mutex_init(&mutexH,NULL) != 0 || pthread_mutex_init(&mutexQ,NULL) != 0) {
		printf("\n mutex init failed.\n");
		return(1);
	}

	

	//"Nettoyage" de la fenêtre console
	//cls();

	readTrans(argv[1]);


	//Fin du programme
	exit(0);
}

