/*
	progetto SOL 2014
	AVL.h
	header file per AVL.c
*/

typedef struct Btree {
	uint64_t key;
	void *info;
	struct Btree *left;
	struct Btree *right;
}Btree_t;

Btree_t *inserisciNodo( Btree_t *t, uint64_t k, void *x );
void *trovaNodo( Btree_t *t, uint64_t k );
void visitaAnticipata( Btree_t *t, void (*f)(uint64_t, void *) );
void visitaSimmetrica( Btree_t *t, void (*f)(uint64_t, void *) );


