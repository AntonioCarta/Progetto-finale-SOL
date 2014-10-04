/*
	PROGETTO FINALE SOL 2014
	autore: Antonio Carta
	supervisor.c	
*/
#include <unistd.h>
#include <strings.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include "common.h"
#include "AVL.h" 

#define SERVERFILE "./server"

typedef struct stima_t {	
	struct timespec ultimoTempo;
	int nserver;
	int stima;
}stima_t;

int *pidFigli; //lista dei pid dei figli. termina con -1
int fdpipe[2]; //file descriptor delle pipe aperte con i processi (solo lettura)
volatile sig_atomic_t bprint = 0; //stampa la tabella se uguale a 1
volatile sig_atomic_t balarm = 0;
volatile sig_atomic_t bexit = 0; //termina processo se uguale a 1

void startserver(int k);
void configSignal();
void waitServer();
Btree_t *aggiornaStima(Btree_t *t, msg_t *m);
void stampaTabella( Btree_t *t );
void uccidiFigli();

int main(int argc, char **argv) {
	int k; //numero di server da lanciare
	
	if( argc!=2 ) {
		printf("usage: %s numserver\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	k = atoi( argv[1] );
	
	configSignal();
	
	/* imposto stdout nella modalità line buffered. Serve nel caso in cui
		stdout sia un file. In questo modo output di server e supervisor 
		non rischiano di venire mischiati nella stessa riga. */
	setvbuf( stdout, NULL, _IOLBF, BUFSIZ );
		
	printf("SUPERVISOR STARTING %d\n", k);
	startserver(k);	
	
	waitServer(k);
	return 0;
}

/* fa partire i k server, che comunicheranno con il supervisor tramite la pipe.
	Inoltre si occupa dell'inizializzazione dell'array pidFigli e chiama atexit
	con la funzione che si occupa di uccidere tutti i figli all'uscita del supervisor */
void startserver(int k) {
	int i;
	char s[10], p[10];
	ec_null( (pidFigli=malloc(sizeof(int)*(k+1))) , "malloc" );
	pidFigli[k] = -1;
	ec_meno1( pipe(fdpipe), "pipe" );		
	for( i=0; i<k; i++ ) {
		int x;
		ec_meno1( (x=fork()), "fork" );
		if( x==0 ) { //figlio
			ec_meno1( close(fdpipe[0]) , "close" );  //chiudo la pipe in lettura
			sprintf( s, "%d", i+1 );
			sprintf( p, "%d", fdpipe[1] ); 	//il file descriptor della pipe viene passato come argomento 
			ec_meno1( execl(SERVERFILE, "server", s, p, NULL ), SERVERFILE );
		} else if(x==-1) 
			PERROR("fork")
		else
			pidFigli[i] = x;
	}
	ec_meno1( close(fdpipe[1]), "close pipe" );
	/* all'uscita del supervisor vogliamo eliminare anche i server creati */
	if(atexit( uccidiFigli )!=0) { 
		fprintf(stderr, "atexit fallita\n");
		exit(EXIT_FAILURE); 
	};
}

/* chiamata prima della exit causata dal doppio SIGINT */
void stampaStimastdout( uint64_t k, void *a ) {
	stima_t *x = (stima_t *)a;
	fprintf(stdout, "SUPERVISOR ESTIMATE %d FOR %" PRIx64 " BASED ON %d\n", x->stima, k, x->nserver);
}

/* setta bexit e bprint, usati nel ciclo principale di waitServer per stampare
	o uscire. Se balarm=0 setta balarm e una nuova sveglia tra un secondo.
	il gestore di SIGALARM semplicemente riinizializza balarm a 0.
	Quindi per verificare se è passato meno di un secondo dall'ultimo SIGINT
	basta verificare che balarm=1 */
void gestoreSIGINT( int sign ) {
	if( balarm == 1 ) {
		bexit = 1;
		return;
	}
	bprint = 1;		
	balarm = 1;
	alarm(1);
}

/* se arriva SIGALARM vuol dire che è passaro un secondo dall'ultimo SIGINT,
	quindi non dobbiamo uscire. */
void gestoreSIGALARM( int sign ) {
	balarm = 0;
}

/* imposta il gestore di SIGINT   */
void configSignal() {
    struct sigaction act1, act2;
	
	//SIGINT
	bzero( &act1, sizeof(act1) );
    act1.sa_handler=gestoreSIGINT;
    sigaction(SIGINT, &act1, NULL);
    
    //SIGALARM
    bzero( &act2, sizeof(act2) );
    act2.sa_handler=gestoreSIGALARM;
    sigaction(SIGALRM, &act2, NULL);
}

/*legge tutti i messaggi in arrivo sulla pipe. Se durante il ciclo arrivano dei
	segnali SIGINT si occupa di verificare le variabili modificate dal gestore
	per poter eseguire le rispettive azioni di uscita e/o stampa tabella */
void waitServer(int k) {
	msg_t *m;
	int c;
	int size=sizeof(msg_t);
	char *buf;
	Btree_t *stime = NULL; //albero delle stime
	
	ec_null( (m=malloc(sizeof(msg_t))), "malloc");
	buf=(char *)m;
	while( (c=read(fdpipe[0], buf, size)) ) { //finchè esistono descrittori aperti in scrittura leggi
		/* se errno=EINTR potremmo aver ricevuto un SIGINT o SIGALARM, 
			quindi non vogliamo terminare il processo. */
		if( c==-1 && errno!=EINTR ) PERROR("read pipe");
		if( c==size ) { //lettura completa
 			printf( "SUPERVISOR ESTIMATE %d FOR %" PRIx64 " FROM %d\n", m->stima, m->client, m->server);			
			stime = aggiornaStima( stime, m );
			
			size = sizeof(msg_t);
			buf = (char *)m;
		}
		//lettura parziale ad es. perchè siamo stati interrotti da un segnale
		else if( c>0 ) {
			size-=c;
			buf+=c;
		}
		//bprint e bexit sono settate dal gestore di SIGINT
		if(bprint) {
			stampaTabella( stime );
			bprint=0;
		}
		if(bexit) { 
			visitaSimmetrica( stime, stampaStimastdout );
			printf("SUPERVISOR EXITING\n");			
			exit(0);
		}
		//printf("---------\n");
		//stampaTabella( stime ); 
	}
}

/* aggiorna l'albero delle stime*/
Btree_t *aggiornaStima(Btree_t *t, msg_t *m) {
	stima_t *new;
	stima_t *x;
	int diff;
	
	ec_null( (new=malloc(sizeof(stima_t))), "malloc" );
	x = (stima_t *) trovaNodo(t, m->client);
	
	/* se x==NULL non abbiamo ancora ricevuto stime per questo client, quindi
		inizializziamo e inseriamo il nuovo nodo */
	if( x==NULL ) {
		new->ultimoTempo = m->ultimoTempo;
		new->stima = m->stima;
		new->nserver = 1;
		return inserisciNodo( t, m->client, new ); 
	}
	
	x->nserver+=1;		
	/* altrimenti aggiorniamo solo se la nuova stima è più piccola */
	if( x->stima > m->stima ) 
		x->stima = m->stima;
	
	//intervallo tra ultimi messaggi ricevuti da due server diversi
	diff = msTimeDiff( x->ultimoTempo, m->ultimoTempo );
	diff = (diff<0) ? -diff : diff;

	if( x->stima > diff )
		x->stima = diff;
	
	//tengo il tempo più grande
	if( msTimeDiff(m->ultimoTempo, x->ultimoTempo) > 0 )
		x->ultimoTempo = m->ultimoTempo;
	
	return t;  
}

void stampaStima( uint64_t k, void *a ) {
	stima_t *x = (stima_t *)a;
	fprintf(stderr, "SUPERVISOR ESTIMATE %d FOR %" PRIx64 " BASED ON %d\n", x->stima, k, x->nserver);
}

/* stampa stime correnti su stderr */
void stampaTabella( Btree_t *t ) {
	visitaSimmetrica( t, stampaStima );
}	

/* invia un segnale SIGKILL ai server che sono stati forkati dal supervisor.
	Viene chiamata all'uscita (registrata con atexit subito dopo aver forkato
	i server) */
void uccidiFigli() {
	int i=0;
	char sock[20];
	
	while( pidFigli[i] != -1 ) {
		/* elimino il socket creato dal server, dato che lo chiudo brutalmente. 
			Sarebbe meglio lasciar pulire al server ma questa è l'unica cosa
			che dovrebbe fare comunque. */ 
		sprintf(sock, "OOB-server-%d", i+1);
		if( unlink(sock)==-1 && errno!=ENOENT )
			perror( sock );
		
		//se errno=ESRCH vuol dire che il server è già stato chiuso.
		if( kill(pidFigli[i++], SIGTERM)==-1 && errno!=ESRCH ) 
			perror("kill");
	}
}


