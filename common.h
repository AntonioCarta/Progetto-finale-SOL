/*
	PROGETTO SOL 2014
	common.h
	il file contiene varie definizioni comuni a tutto il progetto come la 
	conversione da host a network byte order e le macro per la gestione base
	degli errori.	
		
	NOTA: sono presenti alcune definizioni di funzioni. Queste dovrebbero 
		andare in un file a parte.
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <inttypes.h>
#include <arpa/inet.h>

#define MAX_SECRET 3000

#define ec_meno1(c, s) if( (c)==-1 ){ perror(s); exit(EXIT_FAILURE); }
#define ec_null(c, s) if( (c)==NULL ) { perror(s); exit(EXIT_FAILURE); }
#define PERROR(s) { perror(s); exit(EXIT_FAILURE); }

/*
#define DEBUGGING 1
#ifdef DEBUGGING
	#define DEBUGPRINT(...) { fprintf(stderr, "(%s:%d) ", __FILE__, __LINE__); \
								fprintf(stderr,  __VA_ARGS__); }
#else
	#define DEBUGPRINT(...)
#endif
*/

typedef struct msg {
	int stima;
	struct timespec ultimoTempo;
	uint64_t client;
	int server;
}msg_t;

/* succ - prec */
long msTimeDiff(struct timespec succ, struct timespec prec) {
	return (succ.tv_sec - prec.tv_sec)*1000 + (succ.tv_nsec - prec.tv_nsec)/1000000;
}

/* conversione da host a network byte order */
uint64_t hton64( uint64_t x ) {
	int t = 0xAB;
	int isBigEndian = (t == htons(t)) ? 1:0;
	if( isBigEndian ) return x;	
	uint64_t maskhigh = 0x00000000FFFFFFFF;
	uint32_t low = x & maskhigh;
	uint32_t high = x >> 32;
	low = htonl(low);
	high = htonl(high);
	return  (uint64_t)low<<32 | high;
}


/* conversione da network a host byte order */
uint64_t ntoh64( uint64_t x ) {
	int t = 0xAB;
	int isBigEndian = (t == ntohs(t)) ? 1:0;
	if( isBigEndian ) return x;	
	uint64_t maskhigh = 0x00000000FFFFFFFF;
	uint32_t low = x & maskhigh;
	uint32_t high = x >> 32;
	low = ntohl(low);
	high = ntohl(high);
	return (uint64_t)low<<32 | high;
}

