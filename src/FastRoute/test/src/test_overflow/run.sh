#!/usr/bin/env bash

###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, University of California, San Diego.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
###############################################################################

GREEN=0
RED=2

if [ "$#" -ne 2 ]; then
	exit 2
fi

binary=$1
testdir=$2

$binary -no_init run.tcl > test.log 2>&1

gold_wl=$(grep -Eo "[0-9]+\.[0-9]+" golden.overflow)
reported_wl=$(grep -Eo "[0-9]+\.[0-9]+" test.log | tail -2 | head -1)

gold_wl=${gold_wl%.*}
reported_wl=${reported_wl%.*}

difference=0

mkdir -p ../../results/test_overflow
cp test.log ../../results/test_overflow/fastroute.log

if [ $gold_wl -lt $reported_wl ];
then
	gold_wl=$(( $gold_wl*100 ))
	ratio=$(( $gold_wl/$reported_wl ))

	difference=$(( 100-$ratio ))
else
	reported_wl=$(( $reported_wl*100 ))
	ratio=$(( $reported_wl/$gold_wl ))
	difference=$(( 100-$ratio ))
fi

if [ $difference -lt 5 ];
then
	exit $GREEN
else
    echo "     - [ERROR] Test failed. Wirelength difference of $difference%"
	exit $RED
fi
