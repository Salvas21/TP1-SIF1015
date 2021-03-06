#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

struct infoVM{						
	int		noVM;
	unsigned char 	busy; 
	unsigned short * 	ptrDebutVM;							
	};								 

struct noeudVM{			
	struct infoVM	VM;		
	struct noeudVM	*suivant;	
	pthread_mutex_t mutexVM;
	};	
	
void cls(void);
void error(const int exitcode, const char * message);

struct noeudVM * findItem(const int no);
struct noeudVM * findPrev(const int no);

void* addItem();
void* removeItem(void *args);
void* listItems(void *args);
void saveItems(const char* sourcefname);
void* executeFile(void *args);

void* readTrans(char* nomFichier);
void* modifier(void *param);
