/*
	PROGETTO FINALE SOL 2014
	autore: Antonio Carta
	server.c
*/ 
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include "common.h"
#include "AVL.h"

/* usiamo CLOCK_REALTIME perchè è l'unico che è garantito essere presente in
	tutti i sistemi POSIX.
	Per la stima è importante che il clock utilizzato sia system-wide e non 
	dipenda dal processo perchè i tempi dei server sono utilizzati per la stima. */
#define CLOCK CLOCK_REALTIME
/* alcuni errori è preferibili gestirli uscendo dal singolo thread invece che
	fermare tutto il server */
#define ec_meno1T( c, s ) if( (c)==-1 ){ perror(s); pthread_exit(NULL); }

 
int fdpipe;
int idserver;

void startServer();
void *comunicaClient( void *fd);
int aggiornaStima( uint64_t id, long diff );

int main(int argc, char **argv) {	
	if(argc!=3) {
		printf("usage: %s idserver fdpipe\n", argv[0]);
		exit(EXIT_FAILURE);  
	}
	idserver = atoi(argv[1]);
	fdpipe = atoi(argv[2]);
	
	/* imposto stdout nella modalità line buffered. Serve nel caso in cui
	stdout sia un file. In questo modo output di server e supervisor 
	non rischiano di venire mischiati nella stessa riga. */
	setvbuf( stdout, NULL, _IOLBF, BUFSIZ );
	
	printf("SERVER %d ACTIVE\n", idserver);
	startServer();
	
	return 0;
}

void eliminaSocket() {
	char nomeSocket[20];
	sprintf(nomeSocket, "./OOB-server-%d", idserver);
	ec_meno1( unlink(nomeSocket), "unlink socket" );
}

/*ciclo principale del server, si limita ad attendere i client e forka per 
	gestire	ogni nuova connessione */
void startServer() {
	char nomeSocket[20];
	int fdsocket;
	struct sockaddr_un sa;
	
	sprintf(nomeSocket, "./OOB-server-%d", idserver);
	//non possiamo creare un socket se esiste già un file con quel nome
	if( unlink( nomeSocket )==-1 && errno!=ENOENT ) { 
		perror("unlink");
		exit(EXIT_FAILURE);
	}
	ec_meno1( (fdsocket = socket( AF_UNIX, SOCK_STREAM, 0)), "socket");
	
	strcpy( sa.sun_path, nomeSocket );
	sa.sun_family = AF_UNIX;
	ec_meno1( bind(fdsocket, (struct sockaddr *)&sa, sizeof(sa)), "bind");
	ec_meno1( listen(fdsocket, SOMAXCONN), "listen" );
	
	//all'uscita voglio eliminare il socket
	if( atexit(eliminaSocket)!=0 )  
		fprintf(stderr, "errore atexit\n");
	
	while(1) {
		int *newfd;
		pthread_t tid;
		
		ec_null( (newfd=malloc(sizeof(int))), "malloc");
		
		ec_meno1( (*newfd=accept(fdsocket, NULL, 0)) , "accept" );
		printf("SERVER %d CONNECT FROM CLIENT\n", idserver);
		
		//=0 successo !=0 errore
		if( pthread_create( &tid, NULL, comunicaClient, newfd ) )
			fprintf( stderr, "impossibile creare thread\n" );
		/* con la detach il thread libera le risorse occupate al termine dell'
			esecuzione. In questo modo non dobbiamo fare la join. 
			Ovviamente la eseguiamo solo se la creazione ha avuto successo. */	
		else if( pthread_detach( tid ) ) 
			fprintf( stderr, "errore pthread_detach\n" );			
	}
}

/* si occupa di leggere i messaggi del client, attraverso *arg (che è un 
	puntatore ad un file descriptor). Per ogni messaggio ricevuto registro 
	l'intervallo di tempo passato tra esso e l'ultimo ricevuto e chiama anche 
	la funzione per aggiornare la stima */
void *comunicaClient(void *arg) {
	uint64_t x=-1;
	long diff, t;	
	int c;
	int fd = *(int *) arg;
	int bprimo = 0; //ho già letto il primo messaggio?
	msg_t m;
	struct timespec prec, succ, init;
		
	ec_meno1T( clock_gettime(CLOCK, &init), "clock" );
	
	/* inizializzo la stima */ 
	m.stima = -1;
	
	while((c=read(fd, &x, sizeof(x)))>0) {
		x = ntoh64(x); //bisogna convertire da network byte order a host byte order		
		ec_meno1T( clock_gettime(CLOCK, &succ), "clock" );
		/* calcolo la stima solo se ho già ricevuto il primo messggio */
		if( bprimo ) {
			diff = msTimeDiff( succ, prec );
			if( m.stima==-1 || m.stima > diff )
				m.stima = diff;
		}
		prec = succ;
		t = msTimeDiff(succ, init);
		/* printf è thread-safe */
		printf("SERVER %d INCOMING FROM %" PRIx64 " @ %ld\n",  idserver, x, t);
		bprimo=1;
	}
	if( c==-1 ) { 
		perror("read socket");
		pthread_exit(NULL);
	}
	
	/* se m.stima==-1 abbiamo al più un messaggio. Dunque non inviamo la 
		nostra stima al server perchè non possiamo farla. */
	if( m.stima == -1) pthread_exit(NULL); 
	
	m.client = x;
	m.server = idserver;
	m.ultimoTempo = succ;
	
	/* se è maggiore di 3000 è sicuramente almeno il doppio. Quindi proviamo a 
		dividere finchè non otteniamo una stima corretta. */
	c=2;
	if( m.stima > 3000 ) {
		if( m.stima/c < 3000 )
			m.stima = m.stima/c;
		c++;
	}
	
	printf("SERVER %d CLOSING %"PRIx64" ESTIMATE %d\n", m.server, m.client, m.stima);
	
	/* la write è atomica perchè minore della dimensione del buffer della pipe.
		Quindi non abbiamo bisogno di un lock. */
	ec_meno1( write(fdpipe, &m, sizeof(msg_t)) , "write pipe");
	
	ec_meno1( close(fd), "close socket" );
	free(arg);
	pthread_exit(NULL);
	return NULL; //non arriviamo qui perchè usiamo la pthread_exit
}

