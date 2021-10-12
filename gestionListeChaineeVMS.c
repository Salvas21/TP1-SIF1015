//#########################################################
//#
//# Titre : 	Utilitaires Liste Chainee et CVS LINUX Automne 21
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
extern struct noeudVM* head;
//Pointeur de queue de liste pour ajout rapide
extern struct noeudVM* queue;
// nombre de VM actives
extern int nbVM;

//#######################################
//# Recherche un item dans la liste chaînée
//# ENTREE: Numéro de la ligne
//# RETOUR:	Un pointeur vers l'item recherché		
//# 		Retourne NULL dans le cas où l'item
//#			est introuvable
//#
struct noeudVM * findItem(const int no){
	//La liste est vide 
	if ((head==NULL)&&(queue==NULL)) return NULL;

	//Pointeur de navigation
	struct noeudVM * ptr = head;

	if(ptr->VM.noVM==no) // premier noeudVM
		return ptr;
	//Tant qu'un item suivant existe
	while (ptr->suivant!=NULL){

		//Déplacement du pointeur de navigation
		ptr=ptr->suivant;

		//Est-ce l'item recherché?
		if (ptr->VM.noVM==no){
			return ptr;
			}
		}
	//On retourne un pointeur NULL
	return NULL;
	}

//#######################################
//#
//# Recherche le PRÉDÉCESSEUR d'un item dans la liste chaînée
//# ENTREE: Numéro de la ligne a supprimer
//# RETOUR:	Le pointeur vers le prédécesseur est retourné		
//# 		Retourne NULL dans le cas où l'item est introuvable
//#
struct noeudVM * findPrev(const int no){
	//La liste est vide 
	if ((head==NULL)&&(queue==NULL)) return NULL;
	//Pointeur de navigation
	struct noeudVM * ptr = head;
	//Tant qu'un item suivant existe
	while (ptr->suivant!=NULL){

		//Est-ce le prédécesseur de l'item recherché?
		if (ptr->suivant->VM.noVM==no){
			//On retourne un pointeur sur l'item précédent
			return ptr;
		}
		//Déplacement du pointeur de navigation
		ptr=ptr->suivant;
		}
	//On retourne un pointeur NULL
	return NULL;
	}

//#####################################################
//# Ajoute un item a la fin de la liste chaînée de VM
//# ENTREE: 
//#	RETOUR:  
void addItem(){
	//Création de l'enregistrement en mémoire
	struct noeudVM* ni = (struct noeudVM*)malloc(sizeof(struct noeudVM));
//printf("\n noVM=%d busy=%d adr ni=%p", ni->VM.noVM, ni->VM.busy, ni);
//printf("\n noVM=%d busy=%d adrQ deb=%p", ni->VM.noVM, ni->VM.busy,queue);

	//Affectation des valeurs des champs
	ni->VM.noVM	= ++nbVM;
	//printf("\n noVM=%d", ni->VM.noVM);
	ni->VM.busy	= 0;
	//printf("\n busy=%d", ni->VM.busy);
	ni->VM.ptrDebutVM	= (unsigned short*)malloc(sizeof(unsigned short)*65536);
//printf("\n noVM=%d busy=%d adrptr VM=%p", ni->VM.noVM, ni->VM.busy, ni->VM.ptrDebutVM);
//printf("\n noVM=%d busy=%d adrQ=%p", ni->VM.noVM, ni->VM.busy, queue);	
	if ((head == NULL) && (queue == NULL)){//liste vide
	  ni->suivant= NULL;
	  queue = head = ni;
	  return;
	}
	struct noeudVM* tptr = queue;
	ni->suivant= NULL;
	queue = ni;
//printf("\n noVM=%d busy=%d adrQ=%p", ni->VM.noVM, ni->VM.busy, queue);	
	tptr->suivant = ni;
//printf("\n noVM=%d busy=%d adr Queue=%p", ni->VM.noVM, ni->VM.busy,queue);


}

//#######################################
//# Retire un item de la liste chaînée
//# ENTREE: noVM: numéro du noeud a retirer 
void removeItem(const int noVM){
	struct noeudVM * ptr;
	struct noeudVM * tptr;
	struct noeudVM * optr;
	//Vérification sommaire (noVM>0 et liste non vide)	
	if ((noVM<1)||((head==NULL)&&(queue==NULL)))
		return;

	//Pointeur de recherche
	if(noVM==1){
		ptr = head; // suppression du premier element de la liste
	}
	else{
		ptr = findPrev(noVM);
	}
	//L'item a été trouvé
	if (ptr!=NULL){
		
		nbVM--;

		// Memorisation du pointeur de l'item en cours de suppression
		// Ajustement des pointeurs
		if((head == ptr) && (noVM==1)) // suppression de l'element de tete
		{
			if(head==queue) // un seul element dans la liste
			{
				free(ptr->VM.ptrDebutVM);
				free(ptr);
				queue = head = NULL;
				return;
			}
			tptr = ptr->suivant;
			head = tptr;
			free(ptr->VM.ptrDebutVM);
			free(ptr);
		}
		else if (queue==ptr->suivant) // suppression de l'element de queue
		{
			queue=ptr;
			free(ptr->suivant->VM.ptrDebutVM);
			free(ptr->suivant);
			ptr->suivant=NULL;
			return;
		}
		else // suppression d'un element dans la liste
		{
			optr = ptr->suivant;	
			ptr->suivant = ptr->suivant->suivant;
			tptr = ptr->suivant;
			free(optr->VM.ptrDebutVM);
			free(optr);
		}
		
		
		while (tptr!=NULL){ // ajustement des numeros de VM

		//Est-ce le prédécesseur de l'item recherché?
			tptr->VM.noVM--;
			//On retourne un pointeur sur l'item précédent	

		//Déplacement du pointeur de navigation
			tptr=tptr->suivant;
		}
	}
}

//#######################################
//#
//# Affiche les items dont le numéro séquentiel est compris dans une plage
//#
void listItems(const int start, const int end){

	//Affichage des entêtes de colonnes
	printf("noVM  Busy?		Adresse Debut VM                        \n");
	printf("========================================================\n");

	struct noeudVM * ptr = head;			//premier element


	while (ptr!=NULL){

		//L'item a un numéro séquentiel dans l'interval défini
		if ((ptr->VM.noVM>=start)&&(ptr->VM.noVM<=end)){
			printf("%d \t %d \t %p\n",
				ptr->VM.noVM,
				ptr->VM.busy, ptr->VM.ptrDebutVM);
			}
		if (ptr->VM.noVM>end){
			//L'ensemble des items potentiels sont maintenant passés
			//Déplacement immédiatement à la FIN de la liste
			//Notez que le pointeur optr est toujours valide
			ptr=NULL;
			}
		else{
			ptr = ptr->suivant;
		}

	}

	//Affichage des pieds de colonnes
	printf("========================================================\n\n");
	}

