#!/bin/bash

#Initial enters 
echo 
echo

NUM=1

  LINE=""

  while [ 1 ]
  do
	#generate some delay before every line
	#disable it on Z88
    echo -n '\\\\\\\\\\' 


    read LINE || break

	#for 48k and zx81, send line number with REM
    echo "${NUM}e${LINE}" 

	#for zx80, send line number with REM
    #echo "${NUM}y${LINE}"

	#for 128k editor, without line number, without REM
    #echo "${LINE}"

	#for 128k editor, with line number, without REM
    #echo "${NUM} ${LINE}"

	#for 128k editor or z88, with line number, with REM
    #echo "${NUM} REM ${LINE}"

    #echo -n '     '
    NUM=$((NUM+1))
  done < $1


