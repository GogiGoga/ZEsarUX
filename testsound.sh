#!/bin/bash


for DRIVER in pulse alsa sdl dsp null; do 

echo "Driver $DRIVER"
sleep 5

./zesarux --verbose 3 --machine 128k media/spectrum/music.tap --ao $DRIVER
./zesarux --verbose 3 --machine 128k media/spectrum/beeper/Digi_Pop.tap --ao $DRIVER

echo "Testear silencio. Primero en 128k, beeper silence"
echo "Luego pasar a 48k, saltaran los dos silence (porque no habra AY Chip)"
sleep 3

./zesarux --verbose 3 --machine 128k --ao $DRIVER
./zesarux --verbose 3 media/zx81/aydemo.p --ao $DRIVER
./zesarux --verbose 3 media/zx81/ORQUESTA.P --ao $DRIVER
./zesarux --verbose 3 media/z88/lem.epr --ao $DRIVER

done
