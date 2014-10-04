 /* 
	BT
	implementazione degli alberi binari di ricerca
	niente rimozione perchè non serve
*/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define ec_null(c, s) if( (c)==NULL ) { perror(s); exit(EXIT_FAILURE); } 


typedef struct Btree {
	uint64_t key;
	void *info;
	struct Btree *left;
	struct Btree *right;
}Btree_t;

Btree_t *inserisciAggiorna( Btree_t *t, uint64_t k, void *x );
void *trova( Btree_t *t, uint64_t k );
void visitaAnticipata( Btree_t *t, void (*f)(uint64_t, void *) );
void visitaSimmetrica( Btree_t *t, void (*f)(uint64_t, void *) );



/*crea un nuovo nodo contenente x e lo inserisce nell'albero t.
	Se esiste già un nodo con la chiave k non fa nulla.
	restituisce il nuovo albero. */
Btree_t *inserisciNodo( Btree_t *t, uint64_t k, void *x ) {
	Btree_t *new;
	int64_t res;
	
	ec_null( (new=malloc(sizeof(Btree_t))), "malloc" );
	new->key = k;
	new->info = x;
	new->left = NULL;
	new->right = NULL;
	
	if( t==NULL ) return new;
	
	Btree_t *succ=t;
	Btree_t *prec=t;
	while( succ!=NULL ) {
		res = succ->key - new->key;
		prec = succ;
		if( res < 0 ) succ = succ->right;
		else if( res > 0 ) succ = succ->left;
		else {
			free(new);
			return t; 
		}
	}
	res = prec->key - new->key;
	if( res < 0 ) prec->right = new;
	else prec->left = new;
	return t;
}

/*restituisce come risultato il nodo che ha chiave k.
	Se non esiste restituisce NULL */
Btree_t *trovaNodo( Btree_t *t, uint64_t k ) {
	int64_t res;
	while( t!=NULL ) {
		res = t->key - k;
		if( res > 0 ) t = t->left;
		else if( res < 0 ) t = t->right;
		else	return t->info;
	}
	return NULL; //x non è stato trovato
}

/*visita anticipata di t. Esegue f per ogni nodo visitato con il campo info e 
	la chiave passati come argomento*/
void visitaAnticipata( Btree_t *t, void (*f)(uint64_t, void *) ) {
	if( t== NULL ) return;
	f( t->key, t->info );
	visitaAnticipata( t->left, f );
	visitaAnticipata( t->right, f );
}


/*visita simmetrica di t. Esegue f per ogni nodo visitato con il campo info e la
	chiave passati come argomento*/
void visitaSimmetrica( Btree_t *t, void (*f)(uint64_t, void *) ) {
	if( t== NULL ) return;
	visitaSimmetrica( t->left, f );
	f( t->key, t->info );
	visitaSimmetrica( t->right, f );
}

void freeAll( Btree_t *t ) {
	if( t==NULL ) return;
	free(t->info);
	free(t->left);
	free(t->right);
	free(t);
}


