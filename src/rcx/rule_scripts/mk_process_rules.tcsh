#!/bin/tcsh

# This script generates the process parameters (w,s,th) and process variability
# tables necessary to run the command "ext rules_gen"

#arguments:
# 1: Variablity directory (IMPORTANT: has to exist in current directory)
# 2: Output file name to be used as the input to Nefelus ext_rules function

# Required directories and files
# directories M1 and M2 and process.stack under argument 1


#-------------------------------------------------------------------
# delete output file from previous run
#-------------------------------------------------------------------
rm -f $2
#-------------------------------------------------------------------


#-------------------------------------------------------------------
# Append M1 and M2 output files into argement 2
#-------------------------------------------------------------------

foreach i ( 1 2 ) 
	echo "VAR_TABLE $i {" >> $2
	echo "" >> $2
	cd $1/M$i

	cat hi_cw.out  lo_cw.out  cth.out   hi_rw.out lo_rw.out  rth.out p.out>> ../../$2

	cd ../../

	echo "}" >> $2
	echo "" >> $2
end
#-------------------------------------------------------------------


#-------------------------------------------------------------------
# Append nominal process.stack onto $2
#-------------------------------------------------------------------
cat $1/process.stack >> $2
#-------------------------------------------------------------------


