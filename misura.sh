#!/bin/bash

# PROGETTO FINALE SOL 2014
# misura.sh
#Si dovrà realizzare uno script bash di nome misura che, ricevuti come argomenti i nomi di un insieme di file
#contenenti l'output di supervisor, server e client, ne legga e analizzi i contenuti, e stampi delle statistiche su
#quanti secret sono stati correttamente stimati dal supervisor (intendendo per stima corretta un secret stimato
#con errore entro 25 unità rispetto al valore del secret vero), e sull'errore medio di stima.
EPSILON=25
CORRETTE=0
ERRORE=0
NSTIME=0

#array associativi, usiamo l'id del client come chiave
declare -A stimeFinali
declare -A secret

for file in $@; do
	#leggi i file riga per riga e raccogli i dati dei messaggi negli opportuni array
	while read line; do
		ARR=($line)
		case $line in
			#SUPERVISOR ESTIMATE Sid FOR id BASED ON n
			*"BASED "*) 
				stimeFinali[${ARR[4]}]=${ARR[2]} ;;
			#CLIENT id SECRET secret
			"CLIENT "*" SECRET "*) 	
				secret[${ARR[1]}]=${ARR[3]} ;;
		esac
	done <$file  #uso il file come input della read
done

	
#sommo tutti gli errori, conto le stime corrette e il numero totale di stime
#ciclando sull'array secret che contiene i secret dei client(corretti)	



for K in "${!secret[@]}"; do 
	#valore assoluto dell'errore
	DELTA=$(( stimeFinali[$K]-secret[$K] ))
	if (( $DELTA < 0 )); then
		DELTA=$((-$DELTA))
	fi
	ERRORE=$(( $ERRORE + $DELTA ))
	NSTIME=$(( $NSTIME + 1 ))
	if (( $DELTA < $EPSILON )); then
		CORRETTE=$(( $CORRETTE+1 ))
	fi
	#stampo solo i secret sbagliati
	if (( $DELTA > EPSILON )); then
		echo "(err=$DELTA)  $K --> ${secret[$K]} "
	fi
done
	
ERRORE=$(( $ERRORE/$NSTIME ))
CORRETTE=$(($CORRETTE*100/$NSTIME))

echo "numero stime: " $NSTIME
echo "stime corrette: ${CORRETTE}%"
echo "errore medio: ${ERRORE}"


