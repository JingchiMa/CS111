#!/bin/bash

echo | ./lab4b --bogus &> /dev/null
if [[ $? -ne 1 ]]
then
	echo "bogus option not detected"

echo | ./lab4b --scale=W &> /dev/null
if [[ $? -ne 1 ]]
then	
	echo "wrong scle not detected"

# borrowded from scnity check script
./lab4b --period=3 --scale=F --log=LOG <<-EOF
SCALE=C
PERIOD=5
STOP
START
LOG 
OFF
EOF
if [[ $? -ne 0 ]]
then
	echo "basic function fails"

