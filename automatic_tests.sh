#!/bin/bash

#tool to run some different tests to ZEsarUX
#tests are Speccy programs, in tap format, which should return text "RESULT: OK" or "RESULT: ERROR", and then exit the emulator by doing a exit emulator action,
#by using ZEsarUX ZXI hardware debug ports

run_test()
{
	TEMPFILE=`mktemp`
	echo Running $@
	$@ > $TEMPFILE
	RESULTADO=`cat $TEMPFILE|grep RESULT|grep OK`
	if [ $? == 0 ]; then
		echo "Result OK"
		rm -f $TEMPFILE
	else
		echo "Error"
		sleep 5
		echo "Output:"
		echo
		cat $TEMPFILE
		rm -f $TEMPFILE
		exit 1
	fi

}

echo "TBBlue MMU test"
run_test ./zesarux --noconfigfile --hardware-debug-ports --machine tbblue --vo stdout --mmc-file tbblue.mmc --enable-mmc --enable-divmmc-ports extras/media/spectrum/tbblue/testmmu.tap 

#RESULT: OK
