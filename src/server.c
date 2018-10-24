#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include "mex.h"// libreria del prototipo del messaggio 

/*
Giacomo Ferro
21/6/2017
Progetto System Call (Realizzazione Server)

*/

int msgid;//identificatore della coda
struct msqid_ds buf;//da passare alla msgctl (preso da sys/msg.h)

pid_t val;//serve per la SIGUSR2

//prototipo di funzione che interpreta messaggio
void InterpretaMex(message *);

void interruzioneCtrlCoZ(int msg){
	
	puts("\n");
	printf ("\n**SIGNAL DEL PADRE SU CTRL-C o CTRL-Z**: Ho ricevuto il segnale numero %d\n",msg);
	puts("***PRIMA DI TERMINARE ELIMINO LA CODA DEI MESSAGGI***\n");
	puts("\n");
	
	msgctl(msgid,IPC_RMID,NULL);//elimino la coda
	kill(val,SIGKILL);//termino il figlio
	exit(0);
}

void interruzioneCtrlCoZFiglio(int msg){

	exit(0);//il figlio se interrotto con ctrl-c termina
}

void segnaleUsr1(int msg){
	
	puts("\n");
	puts("***SIGUSR1 ATTIVATO***");
	printf("numero del segnale:%d\n",msg);
	puts("\n");

	puts("***CODA ELIMINATA E FIGLIO TERMINATO***");

	msgctl(msgid,IPC_RMID,NULL);//elimino la coda e termino
	kill(val,SIGKILL);
	exit(0);
	
}

void segnaleUsr2(int msg){

	puts("\n");
	puts("***SIGUSR2 ATTIVATO***");
	printf("numero del segnale:%d\n",msg);

	msgctl(msgid,IPC_STAT,&buf);

	if(buf.msg_qnum > 0 ){//se ci sono messaggi allora non posso terminare
		puts("\n");
		puts("Ci sono messaggi nella coda. Eseguire il comando freeall per rimuoverli.");
		puts("***TERMINAZIONE CON SIGUSR2 NON RIUSCITA***");
	}
	else{
		puts("\n");
		puts("***CODA ELIMINATA E FIGLIO TERMINATO***");
		msgctl(msgid,IPC_RMID,NULL);//altrimenti elimino la coda e termino 
		kill(val,SIGKILL);//invio la terminazione al figlio
		exit(0);
	}

}

void attivaUsr1(int msg){

	puts("\n");
	puts("***ALARM ATTIVATA***");
	printf("numero del segnale:%d\n",msg);
	puts("\n");
	
	//serve al figlio per attivare SIGUSR1..
	puts("***LANCIO LA SIGUSR1 DOPO ALARM***");
	kill(getppid(),SIGUSR1);
	exit(0);
	
}

int main(int argc, char * argv[]){
	
	
	message * ptr;//puntatore di tipo messaggio

	msgid=msgget(MSGKEY,(0666|IPC_CREAT|IPC_EXCL));//creo la coda di messaggi con permessi di scrittura e lettura
	if(msgid==-1){
		perror(argv[0]);
		exit(1);
	}
	
	ptr=(message *)malloc(sizeof(message));
	if(ptr==NULL){
		puts("non c'è memoria!");
		msgctl(msgid,IPC_RMID,NULL);
		exit(1);
	}
	
	
	signal(SIGTSTP,interruzioneCtrlCoZ);//segnalo per intercettare ctrl-z
	signal(SIGINT, interruzioneCtrlCoZ);//segnalo il processo padre per catturare ctrl-c
	signal(SIGUSR1, segnaleUsr1);//devo fare in modo di terminare
	signal(SIGUSR2, segnaleUsr2);//devo dare msgctl (msg_qnum sono il numero di messaggi)
	
	puts("***SERVER AVVIATO CORRETTAMENTE***");
	
	val=fork();//creo un figlio
	
	if(val==-1){
		perror("fork() fallita");
		exit(1);
	}
	
	if(val==0){//se sono nel figlio...
		
		signal(SIGTSTP,interruzioneCtrlCoZFiglio);//segnalo il processo figlio per intercettare ctrl-z
		signal(SIGINT, interruzioneCtrlCoZFiglio);//segnalo il processo figlio per catturare ctrl-c
		signal(SIGALRM, attivaUsr1);
		
		while(1){//ciclo infinito finchè non abbiamo una terminazione del programma
			
			puts("\n**********************************************************\n");
			puts("***ATTENDO NUOVI MESSAGGI***");
			puts("");
			
			//vedo se ci sono messaggi di tipo 1...
			if(msgrcv(msgid,ptr,sizeof(message), MSGTYPE1, 0) == -1) {
				perror("msgrcv() fallita");
				msgctl(msgid, IPC_RMID, NULL);
				exit(1);
			}
			
			//se trovo un messaggio devo interpretarlo..
			
			puts("***INTERPRETO IL MESSAGGIO DI TIPO 1 RICEVUTO.***");
			
			InterpretaMex(ptr);//interpreto messaggio tramite funzione..
			
		}//fine di while()
		
	}//fine figlio..
	
	if(val>0){//il padre deve trattare i segnali lanciati dal figlio..

		wait(NULL);//aspetto che il figlio termini
	}
	
}//fine main


void InterpretaMex(message * ptr){

	if(strcmp(ptr->stringa,"niceclose") == 0){
				
		puts("***NICECLOSE ATTIVATO***");
				
		kill(getppid(),SIGUSR2);//segnalo al padre la SIGUSR2 per controllare se può terminare o meno..
		sleep(1);//attendo il ritorno da SIGUSR2
	}
	else if(strcmp(ptr->stringa,"freeall") == 0){//leggo tutti i messaggi
				
		puts("***FREEALL ATTIVATO***");
				
		msgctl(msgid,IPC_STAT,&buf);

		if(buf.msg_qnum > 0 ){//se ci sono messaggi li leggo tutti..
			do{
				if(msgrcv(msgid,ptr,sizeof(message),0,0) == -1){
					perror("msgrcv() fallita");
					msgctl(msgid, IPC_RMID, NULL);
					exit(1);
				}
				msgctl(msgid,IPC_STAT,&buf); //aggiorno buf
			}while(buf.msg_qnum > 0);
			puts("Messaggi eliminati con successo");
		}
		else{
			puts("Non ci sono messaggi presenti in coda");
		}
	}	
	else if(strcmp(strtok(ptr->stringa," \n"),"closetime") == 0){
				
		puts("***CLOSETIME ATTIVATO***");
				
		char * p;
		int n;
						
		p=(char *)malloc(sizeof(char)*5);	
				
		if(p!=NULL){
			p=&((ptr->stringa)[10]);//prendo solo il valore numerico
		}
		else{
			puts("non c'è memoria");
			exit(1);
		}
				
		n=atoi(p);//converto in intero
		alarm(n);//preparo l'allarme	
		
		printf("allarme settato correttamente a %d secondi.\n",n);		
		
		pause();//aspetto il segnale altrimenti abbiamo interruzione delle msgrcv
				
		exit(0);
				
	}

}//fine funzione
