#!/bin/bash

projectRoot=$PWD/..
spotters[0]=192.168.1.100
spotters[1]=192.168.1.101
spotters[2]=192.168.1.102
spotters[3]=192.168.1.103
spotters[4]=192.168.1.104
spotters[5]=192.168.1.105

serverIp=192.168.1.99

cd $projectRoot

# ---------------------------------------------------------------
# Determine spotters to start
# ---------------------------------------------------------------

spottersToBeStarted=()
index=0
for i in "$@"
do
    case $i in
	-s=*|--spotter=*)
	    sps="${i#*=}"
	    for j in ${sps//,/ }
	    do
		echo "going to start spotter $j with ip ${spotters[$j]}"
		spottersToBeStarted[$index]=$j
		((index++))
	    done
		;;
	esac
done

# ---------------------------------------------------------------
# Start beholder
# ---------------------------------------------------------------

cd build/x64
gnome-terminal -- echo "beholder" && ./beholder

# ---------------------------------------------------------------
# Start spotters
# ---------------------------------------------------------------

for spotter in "${spottersToBeStarted[@]}"
do
	gnome-terminal -- ssh -i $projectRoot/keys/spotter_rsa pi@${spotters[$spotter]} "echo \"spotter $spotter\" && cd ./mimikry && ./spotter -s $serverIp"
done
