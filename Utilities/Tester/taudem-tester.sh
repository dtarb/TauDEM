#!/bin/bash

if [ $# -ne 3 ]; then
	echo "USAGE: . taudem-tester.sh [TAUDEM EXEC PATH] [NUMBER OF PROCS] [INPUT DEM]"
	return
fi

clear

taudempath=$1
numprocs=$2
inputdem=$3
defaultinputdem=test.tif
templog=temp.txt

rm -f $defaultinputdem

if [[ $inputdem =~ ^http://.+ ]] || [[ $inputdem =~ ^https://.+ ]]; then
	curl --fail -L $inputdem > $defaultinputdem
	if [ -f $defaultinputdem ]; then
		inputdem=$defaultinputdem
	else
		echo "Failed to download $inputdem"
		return
	fi
elif [ ! -f $inputdem ]; then
	echo "File $inputdem does not exist"
	return
fi

PATH=$taudempath:$PATH

taudem_progs=(
	"pitremove -z $inputdem -fel loganfel.tif"
	)

taudem_benchmark_profiles=(
	"Header read time;Data read time;Compute time;Write time;Total time"
)
	
echo "Inputdem is $inputdem"
echo "TauDEM tests are starting..."
echo

count=0
pcounter=0
for ((count=0; count < ${#taudem_progs[@]}; count++)); do
	IFS=' ' params=(${taudem_progs[count]})
	prog=${params[0]}
	proglog="$prog-benchmark.txt"
	progcsv="$prog-benchmark.csv"

	cp /dev/null $proglog
	cp /dev/null $progcsv

	echo "Starting $prog test..."

	IFS=';' profiles=(${taudem_benchmark_profiles[count]})

	printf "Number of Processors" >> $progcsv
	for profile in ${profiles[@]}; do
		printf ", $profile" >> $progcsv
	done
	echo "" >> $progcsv

	for ((pcounter=1; pcounter <= $numprocs; pcounter++)); do
		mpirun -n $pcounter "${params[@]}" > $templog 2>&1
		cat $templog >> $proglog

		printf "$pcounter" >> $progcsv
		for profile in ${profiles[@]}; do
			columnsize=$(echo "$profile" | wc -w)
			((columnsize++))
			printf ", " >> $progcsv
			grep "$profile" temp.txt | awk -v columnsize=$columnsize '{print $columnsize}' | tr -d '\n' >> $progcsv
		done
		echo "" >> $progcsv
	done

	echo "${taudem_progs[$count]} is finished"
	echo
done

#rm -f $templog

echo "All tests are finished!"
