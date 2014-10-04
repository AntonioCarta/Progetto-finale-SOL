/*
	PROGETTO SOL 2014
	autore: Antonio Carta

*/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include "common.h"

uint64_t idclient;
int secret;
int *serverScelti;

uint64_t rand64bit();
int *scegliServer(int p, int k);
void contattaServer(int p, int w);

int main(int argc, char **argv) {
	int p,k,w;
	struct timeval t;
	
	if( argc!=4 ) {
		printf("usage: %s p k w \n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	p = atoi( argv[1] );
	k = atoi( argv[2] );
	w = atoi( argv[3] );
	
	//condizioni sui parametri
	if( p<1 || p>=k || w<=3*p ) {
		printf("1 ≤ p < k e w > 3p \n");
		exit(EXIT_FAILURE);
	}
	
	gettimeofday( &t, NULL );
	srand(t.tv_usec ^ t.tv_sec );  
	idclient = rand64bit();
	secret = (rand()%MAX_SECRET) + 1;
	printf("CLIENT %" PRIx64 " SECRET %d\n", idclient, secret);
	serverScelti = scegliServer(p, k);
	contattaServer(p, w);
	return 0;
}

/* il minimo garantito per RAND_MAX è 2^16-1 */
uint64_t rand64bit() {
	uint64_t x=0;
	x ^= ((uint64_t)rand() & 0xFFFF);
	x ^= ((uint64_t)rand() & 0xFFFF)<<16;
	x ^= ((uint64_t)rand() & 0xFFFF)<<32;
	x ^= ((uint64_t)rand() & 0xFFFF)<<48;
	return x;	
}

/*crea l'array serverScelti inserendo nelle prime p posizioni i server scelti*/
int *scegliServer(int p, int k) {
	int i, x, t;
	int *a;
	ec_null( (a=malloc(sizeof(int)*k)), "malloc");
	for(i=0; i<k; i++) {
		a[i] = i+1;
	}
	for(i=0; i<k; i++) {
		x = (rand()%(k-i)) + i;
		t = a[x];
		a[x] = a[i];
		a[i] = t;
	}
	return a;
}


void contattaServer(int p, int w) {
	int i;
	int *sock;
	ec_null( (sock=malloc(sizeof(int)*p)), "malloc" );
	//mi connetto con i p server scelti
	for	( i=0; i<p; i++ ) {
		int res;
		struct sockaddr_un sa;
		ec_meno1( (sock[i]=socket(AF_UNIX, SOCK_STREAM, 0)), "socket" );
		sprintf(sa.sun_path, "OOB-server-%d", serverScelti[i]);
		sa.sun_family = AF_UNIX;
		//riprovo finchè il socket non viene creato
		while( (res=connect(sock[i], (struct sockaddr *)&sa, sizeof(sa)))==-1 ) {
			if( errno!=ENOENT )
				PERROR(sa.sun_path);
			sleep(1);
		} 	
	}

	//invio messaggi
	uint64_t idhorder = hton64(idclient);
	struct timespec delta, rem;
	delta.tv_sec = secret/1000; 
	delta.tv_nsec = (secret%1000) * 1000000;
	for ( i=0; i<w; i++ ) {
		ec_meno1( nanosleep( &delta, &rem ), "nanosleep" );
		int x = rand() % p;		
		ec_meno1( write(sock[x], &idhorder, sizeof(uint64_t)), "write socket");
	}
	printf("CLIENT %" PRIx64 " DONE\n", idclient);
}


