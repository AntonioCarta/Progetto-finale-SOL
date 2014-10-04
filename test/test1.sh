#!/bin/bash
BDIR=./build
CLIENT=./client
K=4
P=1
W=4
C=2

cd $BDIR

echo "test"
./supervisor $K &
PID=$!
echo $PID

sleep 1
echo "starting client"
for ((i = 0 ; i < $C ; i++ )); do 	
	$CLIENT $P $K $W &
	sleep 1
done

for ((i = 0 ; i < 2 ; i++ )); do 	
	echo "segnale $i"
	kill -SIGINT $PID
	sleep 10
done

echo "terminazione"
kill -SIGINT $PID
sleep 0.5
kill -SIGINT $PID


