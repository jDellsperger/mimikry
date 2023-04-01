#!/bin/bash

projectRoot=$PWD/..
spotters[0]=192.168.1.100
spotters[1]=192.168.1.101
spotters[2]=192.168.1.102
spotters[3]=192.168.1.103
spotters[4]=192.168.1.104
spotters[5]=192.168.1.105


# ---------------------------------------------------------------
# Determine spotters to deploy to
# ---------------------------------------------------------------

fastDeploy=false
spottersToBeBuilt=()
index=0
for i in "$@"
do
    case $i in

	-s=*|--spotter=*)
	    sps="${i#*=}"
	    for j in ${sps//,/ }
	    do
			echo "going to deploy to spotter $j with ip ${spotters[$j]}"
			spottersToBeBuilt[$index]=$j
			((index++))
	    done
		shift
	;;

	--fast)
		echo "doing fast deploy"
		fastDeploy=true
		shift
	;;

	esac
done

if [ $index == 0 ]; then
    echo "Specify spotters to be deployed to with -s={spotterId1,spotterId2,...}"
    exit 1
fi

# ---------------------------------------------------------------
# Pack libraries and includes
# ---------------------------------------------------------------

mkdir spotter_deploy
mkdir spotter_deploy/mimikry

cp -av $projectRoot/build/arm/* spotter_deploy/mimikry

if [ "$fastDeploy" = false ]; then
	mkdir spotter_deploy/lib
	mkdir spotter_deploy/include
	cp -av $projectRoot/externals/lib/* spotter_deploy/lib
	cp -av $projectRoot/externals/include/* spotter_deploy/include
fi

tar -zcvf spotter_deploy.tar.gz spotter_deploy

# ---------------------------------------------------------------
# Push executables to raspberrys
# ---------------------------------------------------------------

for spotter in "${spottersToBeBuilt[@]}"
do
    scp -i $projectRoot/keys/spotter_rsa spotter_deploy.tar.gz pi@${spotters[$spotter]}:~
    ssh -i $projectRoot/keys/spotter_rsa pi@${spotters[$spotter]} "tar -zxvmf spotter_deploy.tar.gz"
    ssh -i $projectRoot/keys/spotter_rsa pi@${spotters[$spotter]} "mkdir mimikry"
    ssh -i $projectRoot/keys/spotter_rsa pi@${spotters[$spotter]} "mkdir mimikry/calibrations"
    ssh -i $projectRoot/keys/spotter_rsa pi@${spotters[$spotter]} "cp -av spotter_deploy/mimikry/* ~/mimikry"
	
	if [ "$fastDeploy" = false ]; then
	    ssh -i $projectRoot/keys/spotter_rsa pi@${spotters[$spotter]} "sudo cp -av spotter_deploy/lib/* /usr/lib/arm-linux-gnueabihf/"
    	ssh -i $projectRoot/keys/spotter_rsa pi@${spotters[$spotter]} "sudo cp -av spotter_deploy/include/* /usr/include/"
	fi

	ssh -i $projectRoot/keys/spotter_rsa pi@${spotters[$spotter]} "rm -rf spotter_deploy"
    ssh -i $projectRoot/keys/spotter_rsa pi@${spotters[$spotter]} "rm spotter_deploy.tar.gz"
done

# ---------------------------------------------------------------
# Cleanup
# ---------------------------------------------------------------

rm -rf spotter_deploy
rm spotter_deploy.tar.gz
