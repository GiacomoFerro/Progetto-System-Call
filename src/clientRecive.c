#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include "mex.h"// il prototipo del messaggio 

/*
Giacomo Ferro
21/6/2017
Progetto System Call (Realizzazione ClientRecive)

*/

//esegue una msgrcv() con chiamata bloccante se la coda non ha messaggi di tipo >=2

int controlloParametri(int, char * str[]);

//funzione ctrl-z e ctrl-c
void bloccoCtrlCoZ(int msg){
	
	int open;
	message * p;

	puts("\n");	
	printf ("**SIGNAL**: Ho ricevuto il segnale numero %d\n",msg);
	puts("***INTERCETTATO CTRL-C o CTRL-Z PROCEDO SEGNALANDO AL SERVER IL BLOCCO***\n");
	puts("\n");
	
	p=(message *)malloc(sizeof(message));
	if(p!=NULL){
		p->mtype=1;
		strcpy(p->stringa,"closetime 1");//prima parte del testo
	}
	else{
		puts("non c'è memoria!");
		exit(1);
	}
	open=msgget(MSGKEY,0666);//apro la coda
	
	if(open ==-1){
		perror("msgget() fallita");
		exit(1);
	}	
	
	if(msgsnd(open,p,sizeof(message),0)==-1) {//spedisco il messaggio in coda con il tipo indicato
		perror("msgsnd() fallita");
		exit(1);
	}
	exit(1);
}


int main(int argc, char *argv[]){

	message * ptr;
	int msgid;
	int tipo;
	
	signal(SIGINT, bloccoCtrlCoZ);
	signal(SIGTSTP, bloccoCtrlCoZ);
	
	if(!controlloParametri(argc,argv)){
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
	
	//leggo i messaggi di tipo diverso da 1!!
	
	sleep(1);//do il tempo di digitare ctrl-z o ctrl-z...
	
	tipo=atol(argv[1]);
	
	if(msgrcv(msgid,ptr,sizeof(message),tipo,0) == -1){
		perror("msgrcv() fallita");
		exit(1);
	}
	
	printf("%s\n",ptr->stringa);//stampo il messaggio di tipo > 1
	
}//fine main


int controlloParametri(int argc, char * argv[] ){

	if(argc!=2){
		puts("PARAMETRI NON INSERITI CORRETTAMENTE. (SERVE NUMERO DEL MAILBOX DA CUI LEGGERE I MESSAGGI).");
		return 0;
	}
	if(atoi(argv[1])< 2){//se valore per il mailbox <2 allora errore
		puts("VALORE PER MAILBOX NON VALIDO. (DEVE ESSERE >=2).");
		return 0;
	}

	return 1;//in caso di successo..

}//fine funzione
