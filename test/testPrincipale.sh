#!/bin/bash

#test, lanciando il supervisor con 8 server, e dopo un'attesa di 2 secondi dovrà lanciare un totale di
#20 client, ciascuno con parametri 5 8 20. I client andranno lanciati a coppie, e con un attesa di 1 secondo fra
#ogni coppia.
#Dopo aver lanciato l'ultima coppia, il test dovrà attendere 60 secondi, inviando nel frattempo un SIGINT al
#supervisor ogni 10 secondi; al termine dovrà inviare un doppio SIGINT, e lanciare lo script misura sui
#risultati raccolti (tramite redirezione dello stdout dei vari componenti su appositi file)
BDIR=./build
CLIENT=./client
OUTCLI=outclient
OUTSERV=outserver
OUTMISURA=outMisura
K=8
P=5
W=20
C=10


cd $BDIR

echo "eseguo supervisor"
./supervisor $K >$OUTSERV &
PID=$!

sleep 2

#elimino vecchi file per non avere dati sui vecchi client
rm $OUTCLI

echo "eseguo client"
for ((i = 0 ; i < $C ; i++ )); do 	
	$CLIENT $P $K $W >>$OUTCLI &
	$CLIENT $P $K $W >>$OUTCLI &
	sleep 1
done

echo "invio segnali"
for ((i = 0 ; i < 10 ; i++ )); do
	echo "segnale $i"
	kill -SIGINT $PID
	sleep 10
done

echo "terminazione"
kill -SIGINT $PID
sleep 0.5
kill -SIGINT $PID


echo ------------------- 
echo "statistiche stime"
echo "K=$K P=$P W=$W"
echo -------------------

../misura.sh $OUTSERV $OUTCLI
