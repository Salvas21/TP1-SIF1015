//#########################################################
//#
//# Titre : 	Utilitaires CVS LINUX Automne 21
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
extern struct noeud* head;
//Pointeur de queue de liste pour ajout rapide
extern struct noeud* queue;

// nombre de VM actives
extern int nbVM;

//#######################################
//#
//# Affiche une série de retour de ligne pour "nettoyer" la console
//#
void cls(void){
	printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
	}

//#######################################
//#
//# Affiche un messgae et quitte le programme
//#
void error(const int exitcode, const char * message){
	printf("\n-------------------------\n%s\n",message);
	exit(exitcode);
	}
	
/* Sign Extend */
uint16_t sign_extend(uint16_t x, int bit_count)
{
    if ((x >> (bit_count - 1)) & 1) {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

/* Swap */
uint16_t swap16(uint16_t x)
{
    return (x << 8) | (x >> 8);
}

/* Update Flags */
void update_flags(uint16_t reg[R_COUNT], uint16_t r)
{
    if (reg[r] == 0)
    {
        reg[R_COND] = FL_ZRO;
    }
    else if (reg[r] >> 15) /* a 1 in the left-most bit indicates negative */
    {
        reg[R_COND] = FL_NEG;
    }
    else
    {
        reg[R_COND] = FL_POS;
    }
}

/* Read Image File */
int read_image_file(uint16_t * memory, char* image_path,uint16_t * origin)
{
	 char fich[200];
	 strcpy(fich,image_path);
  	 FILE* file = fopen(fich, "rb");
 
    if (!file) { return 0; }
    /* the origin tells us where in memory to place the image */
   	*origin=0x3000;

    /* we know the maximum file size so we only need one fread */
    uint16_t max_read = UINT16_MAX - *origin;
    uint16_t* p = memory + *origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);
    /* swap to little endian ???? */
    while (read-- > 0)
    {
    //	printf("\n p * BIG = %x",*p);
       // *p = swap16(*p);
		// printf("\n p * LITTLE = %x",*p);
        ++p;
    }
    return 1;
}


/* Check Key */
uint16_t check_key()
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &readfds, NULL, NULL, &timeout) != 0;
}

/* Memory Access */
void mem_write(uint16_t * memory, uint16_t address, uint16_t val)
{
    memory[address] = val;
}

uint16_t mem_read( uint16_t * memory, uint16_t address)
{
    if (address == MR_KBSR)
    {
        if (check_key())
        {
            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = getchar();
        }
        else
        {
            memory[MR_KBSR] = 0;
        }
    }
    return memory[address];
}

/* Input Buffering */
struct termios original_tio;

void disable_input_buffering()
{
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

/* Handle Interrupt */
void handle_interrupt(int signal)
{
    restore_input_buffering();
    printf("\n");
    exit(-2);
}



//#######################################
//#
//# Execute le fichier de code .obj 
//#
void* executeFile(void* args){
char sourcefname[100];
int noVM;

noVM = ((struct paramX*)args)->noVM;
strcpy(sourcefname, (const char *) ((struct paramX*)args)->nomfich);

free(args);
/* Memory Storage */
/* 65536 locations */
	uint16_t * memory;
	uint16_t origin;
	uint16_t PC_START;
	
/* Register Storage */
	uint16_t reg[R_COUNT];
	
    struct noeudVM * ptr =  findItem(noVM);
	
    if(ptr == NULL)
    {
        printf("Virtual Machine unavailable\n");
        pthread_exit(0);
    }	

	memory = ptr->VM.ptrDebutVM;
    if (!read_image_file(memory, sourcefname, &origin))
    {
        printf("Failed to load image: %s\n", sourcefname);
        pthread_mutex_unlock(&ptr->mutexVM);
        pthread_exit(0);
    }
	
    while(ptr->VM.busy != 0){ // wait for the VM 
    }
	// Acquiring access to the VM
    ptr->VM.busy = 1;
    
    /* Setup */
    signal(SIGINT, handle_interrupt);
    disable_input_buffering();

    /* set the PC to starting position */
    /* at  ptr->VM.ptrDebutVM + 0x3000 is the default  */
    //enum { PC_START = origin };
    PC_START = origin;
    reg[R_PC] = PC_START;

    int running = 1;
    while (running)
    {
        /* FETCH */
        uint16_t instr = mem_read(memory, reg[R_PC]++);
// printf("\n instr = %x", instr);
        uint16_t op = instr >> 12;
	
// printf("\n exe op = %x", op);

        switch (op)
        {
            case OP_ADD:
                /* ADD */
                {
                    /* destination register (DR) */
                    uint16_t r0 = (instr >> 9) & 0x7;
                    /* first operand (SR1) */
                    uint16_t r1 = (instr >> 6) & 0x7;
                    /* whether we are in immediate mode */
                    uint16_t imm_flag = (instr >> 5) & 0x1;
                
                    if (imm_flag)
                    {
                        uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                        reg[r0] = reg[r1] + imm5;
                    }
                    else
                    {
                        uint16_t r2 = instr & 0x7;
                        reg[r0] = reg[r1] + reg[r2];
 printf("\n add reg[r0] (sum) = %d", reg[r0]);
 //printf("\t add reg[r1] (sum avant) = %d", reg[r1]);
 //printf("\t add reg[r2] (valeur ajoutee) = %d", reg[r2]);
                    }
                
                    update_flags(reg, r0);
                }

                break;
            case OP_AND:
                /* AND */
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t r1 = (instr >> 6) & 0x7;
                    uint16_t imm_flag = (instr >> 5) & 0x1;
                
                    if (imm_flag)
                    {
                        uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                        reg[r0] = reg[r1] & imm5;
                    }
                    else
                    {
                        uint16_t r2 = instr & 0x7;
                        reg[r0] = reg[r1] & reg[r2];
                    }
                    update_flags(reg, r0);
                }

                break;
            case OP_NOT:
                /* NOT */
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t r1 = (instr >> 6) & 0x7;
                
                    reg[r0] = ~reg[r1];
                    update_flags(reg, r0);
                }

                break;
            case OP_BR:
                /* BR */
                {
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    uint16_t cond_flag = (instr >> 9) & 0x7;
                    if (cond_flag & reg[R_COND])
                    {
                        reg[R_PC] += pc_offset;
                    }
                }

                break;
            case OP_JMP:
                /* JMP */
                {
                    /* Also handles RET */
                    uint16_t r1 = (instr >> 6) & 0x7;
                    reg[R_PC] = reg[r1];
                }

                break;
            case OP_JSR:
                /* JSR */
                {
                    uint16_t long_flag = (instr >> 11) & 1;
                    reg[R_R7] = reg[R_PC];
                    if (long_flag)
                    {
                        uint16_t long_pc_offset = sign_extend(instr & 0x7FF, 11);
                        reg[R_PC] += long_pc_offset;  /* JSR */
                    }
                    else
                    {
                        uint16_t r1 = (instr >> 6) & 0x7;
                        reg[R_PC] = reg[r1]; /* JSRR */
                    }
                    break;
                }

                break;
            case OP_LD:
                /* LD */
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    reg[r0] = mem_read(memory, reg[R_PC] + pc_offset);
                    update_flags(reg, r0);
                }

                break;
            case OP_LDI:
                /* LDI */
                {
                    /* destination register (DR) */
                    uint16_t r0 = (instr >> 9) & 0x7;
                    /* PCoffset 9*/
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    /* add pc_offset to the current PC, look at that memory location to get the final address */
                    reg[r0] = mem_read(memory, mem_read(memory, reg[R_PC] + pc_offset));
                    update_flags(reg, r0);
                }

                break;
            case OP_LDR:
                /* LDR */
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t r1 = (instr >> 6) & 0x7;
                    uint16_t offset = sign_extend(instr & 0x3F, 6);
                    reg[r0] = mem_read(memory, reg[r1] + offset);
                    update_flags(reg, r0);
                }

                break;
            case OP_LEA:
                /* LEA */
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    reg[r0] = reg[R_PC] + pc_offset;
                    update_flags(reg, r0);
                }

                break;
            case OP_ST:
                /* ST */
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    mem_write(memory, reg[R_PC] + pc_offset, reg[r0]);
                }

                break;
            case OP_STI:
                /* STI */
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                    mem_write(memory, mem_read(memory, reg[R_PC] + pc_offset), reg[r0]);
                }

                break;
            case OP_STR:
                /* STR */
                {
                    uint16_t r0 = (instr >> 9) & 0x7;
                    uint16_t r1 = (instr >> 6) & 0x7;
                    uint16_t offset = sign_extend(instr & 0x3F, 6);
                    mem_write(memory, reg[r1] + offset, reg[r0]);
                }

                break;
            case OP_TRAP:
                /* TRAP */
                switch (instr & 0xFF)
                {
                    case TRAP_GETC:
                        /* TRAP GETC */
                        /* read a single ASCII char */
                        reg[R_R0] = (uint16_t)getchar();

                        break;
                    case TRAP_OUT:
                        /* TRAP OUT */
                        putc((char)reg[R_R0], stdout);
                        fflush(stdout);

                        break;
                    case TRAP_PUTS:
                        /* TRAP PUTS */
                        {
                            /* one char per word */
                            uint16_t* c = memory + reg[R_R0];
                            while (*c)
                            {
                                putc((char)*c, stdout);
                                ++c;
                            }
                            fflush(stdout);
                        }

                        break;
                    case TRAP_IN:
                        /* TRAP IN */
                        {
                            printf("Enter a character: ");
                            char c = getchar();
                            putc(c, stdout);
                            reg[R_R0] = (uint16_t)c;
                        }

                        break;
                    case TRAP_PUTSP:
                        /* TRAP PUTSP */
                        {
                            /* one char per byte (two bytes per word)
                               here we need to swap back to
                               big endian format */
                            uint16_t* c = memory + reg[R_R0];
                            while (*c)
                            {
                                char char1 = (*c) & 0xFF;
                                putc(char1, stdout);
                                char char2 = (*c) >> 8;
                                if (char2) putc(char2, stdout);
                                ++c;
                            }
                            fflush(stdout);
                        }

                        break;
                    case TRAP_HALT:
                        /* TRAP HALT */
                        puts("\n HALT");
                        fflush(stdout);
                        running = 0;

                        break;
                }

                break;
            case OP_RES:
            case OP_RTI:
            default:
                /* BAD OPCODE */
                abort();

                break;
        }
    }
    ptr->VM.busy = 0;
    /* Shutdown */
    restore_input_buffering();

    pthread_mutex_unlock(&ptr->mutexVM);
    pthread_exit(1);
}


//#######################################
//#
//# fonction utilisée pour le traitement  des transactions
//# ENTREE: Nom de fichier de transactions 
//# SORTIE: 
void* readTrans(char* nomFichier){
	FILE *f;
	char buffer[100];
	char *tok, *sp;

    pthread_t tid[1000];
	int nbThread = 0;

	//Ouverture du fichier en mode "r" (equiv. "rt") : [r]ead [t]ext
	f = fopen(nomFichier, "rt");
	if (f==NULL)
		error(2, "readTrans: Erreur lors de l'ouverture du fichier.");

	//Lecture (tentative) d'une ligne de texte
	fgets(buffer, 100, f);

	//Pour chacune des lignes lues
	while(!feof(f)){

		//Extraction du type de transaction
		tok = strtok_r(buffer, " ", &sp);

		//Branchement selon le type de transaction
		switch(tok[0]){
			case 'A':
			case 'a':{
				//Appel de la fonction associée
				//addItem(); // Ajout de une VM
                nbThread = nbThread + 1;
                pthread_create(&tid[nbThread], NULL, addItem, NULL);
				break;
				}
			case 'E':
			case 'e':{
				//Extraction du paramètre
				int noVM = atoi(strtok_r(NULL, " ", &sp));
                
                // typedef struct {
                //     int* noVM;
                // } test_struct;

                int *args = malloc(sizeof *args);
                //args->noVM = &noVM;
                *args = noVM;
				//Appel de la fonction associée
				//removeItem(noVM); // Eliminer une VM
                nbThread++;
                pthread_create(&tid[nbThread], NULL, removeItem, args);
				break;
				}
			case 'L':
			case 'l':{
				//Extraction des paramètres
				int nstart = atoi(strtok_r(NULL, "-", &sp));
				int nend = atoi(strtok_r(NULL, " ", &sp));
				//Appel de la fonction associée
				//listItems(nstart, nend); // Lister les VM

                struct test_struct *args;
                args = malloc(sizeof(struct test_struct));
                args->nstart = nstart;
                args->nend = nend;

                nbThread++;
                pthread_create(&tid[nbThread], NULL, listItems, args);
                for(int i=1;i<nbThread;i++)
                    pthread_join(tid[i], NULL);
				break;
				}
			case 'X':
			case 'x':{
				//Appel de la fonction associée
				int noVM = atoi(strtok_r(NULL, " ", &sp));
				char *nomfich = strtok_r(NULL, "\n", &sp);
 
                struct paramX *args;
                args = malloc(sizeof(struct paramX));
                args->noVM = noVM;
                strcpy(args->nomfich, (const char *) nomfich);
                nbThread++;
                pthread_create(&tid[nbThread], NULL, executeFile, args);
				break;
				}
        //     // case 'M':
        //     // case 'm':{
        //     //     int noVM = atoi(strtok_r(NULL, " ", &sp));
        //     //     // check how to get second param
        //     //     int Lmem = atoi(strtok_r(NULL, "\n", &sp));
        //     //     struct paramMod *ptr = (struct paramMod*) malloc(sizeof(struct paramMod));
        //     //     ptr->noVM = noVM;
        //     //     ptr->Lmem = Lmem;
        //     //     pthread_create(&tid[nbThread++], NULL, modifier, ptr);
        //     // }
		}
		//Lecture (tentative) de la prochaine ligne de texte
		fgets(buffer, 100, f);
	}
	//Fermeture du fichier
	fclose(f);

    for(int i=1;i<nbThread;i++)
        pthread_join(tid[i], NULL);
	//Retour
	return NULL;
}


