CC=gcc
CFLAGS=-Wall

EXC=$(BD)/supervisor $(BD)/client $(BD)/server
OBJ=$(patsubst %, %.o, $(EXC)) AVL.o
BD=./build
.PHONY: all test clean cleanall

all: $(EXC)
	
$(BD)/supervisor: $(BD)/supervisor.o $(BD)/AVL.o
	$(CC) -o $@ -lrt $^
	
$(BD)/client:	$(BD)/client.o
	$(CC) -o $@  -lrt $^
	
$(BD)/server: $(BD)/server.o $(BD)/AVL.o
	$(CC) -o $@  -lrt -lpthread $^

$(BD)/%.o: %.c common.h AVL.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
test: 
	echo "eseguo test"
	./test/testPrincipale.sh
	
clean:
	rm -f $(OBJ) *~ $(BD)/*~ $(BD)/OOB-server-*
	
cleanall :	
	rm -i $(BD)/*	

	
