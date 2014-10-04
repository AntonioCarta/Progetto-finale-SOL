/*
	test alberi binari
*/
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include "AVL.h"


void f( uint64_t k, void *a ) {
	int x = (int)a;
	printf("%"PRId64": %d \n", k, x);
}

int main() {
	Btree_t *t=NULL;
	int i;
	
	for(i=0; i<10; i++) {
		int x;
		printf("ins: ");
		scanf("%d\n", &x);
		t = inserisciAggiorna( t, x, (void *)i ); 
	}
	printf("\n\n");
	visitaAnticipata( t, f );
	printf("\n\n");
	visitaSimmetrica( t, f );
	
	int x;
	printf("trova: ");
	scanf("%d", &x);
	printf("%d\n", (int)trovaNodo(t, x));
	
	freeAll(t);
	
	return 0;
}
