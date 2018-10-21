//definisco messaggio e valori identificativi per i tre programmi .c

/*

VR389882
Giacomo Ferro
21/6/2017
Progetto System Call (Realizzazione libreria messaggio )

*/

#define MSGKEY   88		// chiave per la coda 
#define MSGTYPE1  1		// tipo di messaggio che il figlio (in server.c) deve elaborare nella gestione della coda

typedef struct message{

	long mtype;
	char stringa[256];	

}message;
