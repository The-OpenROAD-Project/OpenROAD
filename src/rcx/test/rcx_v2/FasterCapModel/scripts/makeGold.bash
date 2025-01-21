#!/bin/bash -f
if [ $# -lt 1 ] 
then
	echo "Usage <dir>"
	exit
fi
dir=$1
gold=GOLD

rm -r $dir.$gold
mv $dir $dir.$gold
chmod -w $dir.$gold

