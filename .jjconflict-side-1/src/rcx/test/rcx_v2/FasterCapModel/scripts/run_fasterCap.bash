#!/bin/bash -f
# python_script=~/z/72424/scripts/UniversalFormat2FasterCap.py
# works python_script=~/z/72424/scripts/UniversalFormat2FasterCap_923.py
# works fasterCap=/home/dimitris-ic/fasterCap/920/FasterCAP_v2/FasterCap_v2/build_fasterCap_920/FasterCap

#README - DKF - 092524 : change ext_y and ext_z from 0 to ext_x

if [ $# -lt 7 ] 
then
	echo "Usage <in_dir> <out_dir> <standard|normalized> <ext> <converter_python> <fasterCap_exec>"
	exit
fi
in_dir=$1
outDir=$2
std_normal=$3
ext_x=$4
ext_y=$4
ext_z=$4
pattern=$5
python_script=$6
fasterCap=$7
error=0.01
cd $in_dir
find . -name wires -print | sort > wires_file_list

cd ../
out_dir=$outDir.$std_normal.$ext_x.$ext_y.$ext_z.$pattern
echo "out_dir=$outDir.$std_normal.$ext_x.$ext_y.$ext_z.$pattern"
mkdir -p $out_dir
out=$out_dir/OUT

echo "$out" > $out

for ii in `cat $in_dir/wires_file_list`
do
	dirName=`dirname $ii`
	#if [[ "$dirName" != *"$pattern"*]  && [$pattern!="ALL" ] ] ; then
	if [ "$pattern" != "ALL" ] ; then
		if [[ "$dirName" != *"$pattern"* && $pattern!="ALL" ]] ; then
			continue
		fi
	fi
	echo "Running $dirName ... "

	# incrementality -- if wires.log has total allocated memore greater than 100MB, skip pattern
        wires_log=$out_dir/$dirName/wires.log
        if [ -e $wires_log ]; then
                ls -ltr -h $wires_log
                continue
                echo "     exists   $wires_log "
                bytes=`grep "Total allocated memory" $wires_log | awk '{print $4}'`
                if [ $bytes -gt 100000 ]
                then
                        ls -ltr -h $wires_log
                        continue
                fi
        fi
	echo "python3 $python_script $in_dir/process.out $in_dir/$dirName $out_dir $std_normal -sim_window_ext -$ext_x -$ext_z -$ext_y $ext_x $ext_z $ext_y" >> $out
	python3 $python_script $in_dir/process.out $in_dir/$dirName $out_dir $std_normal -sim_window_ext  -$ext_x -$ext_z -$ext_y $ext_x $ext_z $ext_y >> $out

	# echo "$fasterCap -b $out_dir/$dirName/wires.lst -g -ap -a$error > $out_dir/$dirName/wires.log" >> $out
	# $fasterCap -b $out_dir/$dirName/wires.lst -g -ap -a$error > $out_dir/$dirName/wires.log
	echo "$fasterCap -b $out_dir/$dirName/wires.lst -g -a$error > $out_dir/$dirName/wires.log" >> $out
	$fasterCap -b $out_dir/$dirName/wires.lst -g -a$error > $out_dir/$dirName/wires.log
	echo "Completed `date` $dirName "
done

