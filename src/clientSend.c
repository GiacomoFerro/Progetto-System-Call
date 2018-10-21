#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h> // per funzioni su stringhe
#include <ctype.h> // per isdigit()
#include "mex.h"// il prototipo del messaggio 

/*

VR389882
Giacomo Ferro
21/6/2017
Progetto System Call (Realizzazione ClientSend)

*/

//invia a mailbox con valore specificato dall' utente un messaggio di massimo 256 caratteri
//funzione per gestione di ctrl-z e ctrl-c

int controlloStringhe(char stringa[]);//funzione per il controllo dei comandi per mailbox1
int controlloParametri(int, char * str[]);//funzione per il controllo generale dei parametri

void bloccoCtrlCoZ(int msg){
	
	int open;
	message * p;
	
	puts("\n");
	printf ("**SIGNAL**: Ho ricevuto il segnale numero %d\n",msg);
	puts("***INTERCETTATO CTRL-Z o CTRL-C PROCEDO SEGNALANDO AL SERVER IL BLOCCO***\n");
	puts("\n");
	
	p=(message *)malloc(sizeof(message));
	if(p!=NULL){
		p->mtype=1;
		strcpy(p->stringa,"closetime 1");//serve per settare l'allarme a 1 e terminare correttamente
	}
	else{
		puts("non c'è memoria!");
		exit(1);
	}
	open=msgget(MSGKEY,0666);//apro la coda
	
	if(open == -1){
		perror("msgget() fallita");
		exit(1);
	}
	if(msgsnd(open,p,sizeof(message),0) == -1){//spedisco il messaggio in coda con il tipo indicato
		perror("msgsnd() fallita");
		exit(1);
	}
	exit(1);
	
}

int main(int argc, char *argv[]) {

	message * ptr;
	int msgid;
	int i;
	
	signal(SIGINT, bloccoCtrlCoZ);
	signal(SIGTSTP, bloccoCtrlCoZ);
	
	if(!controlloParametri(argc,argv)){//se ritorna zero allora non proseguo
		puts("\n***TERMINAZIONE DEL PROGRAMMA***");
		exit(1);
	}
	
	/*se supera i controlli procedo..*/

	ptr=(message *)malloc(sizeof(message));
	
	if(ptr==NULL){
		puts("no memoria!");
		exit(1);
	}

	msgid=msgget(MSGKEY,0666);//apro la coda

	if(msgid==-1){
		perror(argv[0]);
		exit(1);
	}

	//preparo il messaggio da inviare
	ptr->mtype=atol(argv[1]);
	strcpy(ptr->stringa,argv[2]);//prima parte del testo
	i=3;
	
	if(argc>=4){//caso di closetime <n> o di un messaggio standard con più token
		while(i<argc){//metto tutti i restanti parametri nella stringa
			strcat(ptr->stringa, " ");//metto uno spazio tra i token
			strcat(ptr->stringa, argv[i]);
			i++;
		}
	}
	
	sleep(1);//do il tempo eventuale di digitare ctrl-z o ctrl-c...
	
	if(msgsnd(msgid,ptr,sizeof(message),0)==-1) {//spedisco il messaggio in coda con il tipo indicato
		perror("msgsnd() fallita");
		exit(1);
	}
	
}//fine main


int controlloStringhe(char stringa[]){

	return strcmp(stringa,"freeall")&&strcmp(stringa,"closetime")&&strcmp(stringa,"niceclose");
	//in caso di successo in una delle tre strmcp() ritorna zero altrimenti >0 o <0
}

int controlloParametri(int argc, char * argv[]){
	
	int i=2;
	char str[256];//vettore dimensione massima del messaggio
	
	while(i<argc){
		strcat(str,argv[i]);
		strcat(str," ");
		i++;
	}

	//controlli sui parametri forniti in generale..
	if(argc<3){
		puts("PARAMETRI NON INSERITI CORRETTAMENTE. (SERVE NUMERO MAILBOX E POI LA STRINGA DEL MESSAGGIO).");
		return 0;
	}
	if(atoi(argv[1])< 1){//se valore per il mailbox <1 allora errore
		puts("VALORE PER MAILBOX NON VALIDO. (DEVE ESSERE >=1).");
		return 0;
	}
	
	if(strlen(str) > 256 ){
		puts("MESSAGGIO INSERITO TROPPO LUNGO.");
		return 0;	
	}
	
	//controlli per il caso mailbox 1..
	if(atoi(argv[1]) == 1){
		if(controlloStringhe(argv[2]) != 0){
			puts("HAI INSERITO UN COMANDO NON PREVISTO PER MAILBOX 1.");
			return 0;
		}
	
		//controllo su closetime
		if(strcmp(argv[2], "closetime") == 0 ){
			if(argv[3]==NULL){
				puts("SINTASSI ERRATA PER CLOSETIME. (MANCA L'INTERO PER I SECONDI).");
				return 0;
			}
			if(!isdigit(*argv[3])){ //se isdigit(*argv[3])==0 allora argv[3] non è un numero oppure è <0.
				puts("SINTASSI ERRATA PER CLOSETIME. (NON HAI INSERITO UN NUMERO PER I SECONDI VALIDO).");
				return 0;
			}
			if(isdigit(*argv[3])&&atoi(argv[3])==0){//caso in cui l'utente inserisca zero come valore per l'alarm()!
				puts("SINTASSI ERRATA PER CLOSETIME. (HAI INSERITO ZERO COME NUMERO DI SECONDI).");
				return 0;
			}
		}
		//controllo se freeall o niceclose hanno troppi parametri
		if((strcmp(argv[2], "freeall")==0 || strcmp(argv[2],"niceclose")==0) && argc>=4){
			puts("SINTASSI ERRATA PER NICECLOSE O FREEALL. (TROPPI PARAMETRI).");
			return 0;
		}
	}
	
	return 1;//se tutto è corretto ritorno 1..
}//fine funzione

