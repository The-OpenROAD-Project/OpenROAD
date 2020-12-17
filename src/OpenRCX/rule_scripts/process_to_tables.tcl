#
# BSD 3-Clause License
#
# Copyright (c) 2019, Nefelus Inc
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#!/bin/env tclsh
#

#---------------------------------------------------------------------
# converts the process tables into Nefelus Variability Tables
#---------------------------------------------------------------------

#---------------------------------------------------------------------
# Required directories
#---------------------------------------------------------------------

# M1      : contains all related numbers for variability for M1
#            dtr, dtc, dwc, dwr, alpha, x, p
# M2      : contains all related numbers for variability for M2-3-4-5
#            dtr, dtc, dwc, dwr, alpha, x, p

#---------------------------------------------------------------------
# Output files for each M1 and M2
#---------------------------------------------------------------------
#            lo_cw.out - lo width for C
#            hi_cw.out - hi width for C
#            lo_rw.out - lo width for R
#            hi_rw.out - hi width for R
#            cth.out - thickness for C
#            rth.out - thickness for R

proc printArray1d { AA name } {
	upvar $AA A
	set s1 [array size A]
	
	variable out ""
	for { set i 0 } { $i < $s1 } { incr i } {
		lappend out $A($i)
	}
	puts "$name $out"
}
proc printArray1df { fp AA name } {
	upvar $AA A
	set s1 [array size A]
	
	variable out ""
	for { set i 0 } { $i < $s1 } { incr i } {
		lappend out $A($i)
	}
	puts $fp "$name $out"
}
proc makeEffP { dir filename rows name1 cols name2 } {
	set datafile [file join $dir $filename ]
	set fp [open $datafile "w"]
	upvar $rows W
	upvar $cols P

	printArray1df $fp W $name1
	printArray1df $fp P $name2
	close $fp
}
proc makeEffWidth { dir filename rows name1 cols name2 val name3 } {

	set datafile [file join $dir $filename ]
	set fp [open $datafile "w"]

	upvar $rows R
	upvar $cols C
	upvar $val  V

	printArray1df $fp R $name1
	printArray1df $fp C $name2

	puts $fp $name3

	for { set i 0 } { $i < [array size R] } { incr i } {
		set w $R($i)

		variable row ""
		for { set j 0 } { $j < [array size C] } { incr j } {
			set s $C($j)
        		set DWC $V($i,$j)

        		#puts "i= $i  j= $j  w= $w  s= $s   dwc= $DWC"

			set cW [ expr $w - 2*$DWC ]
			lappend row $cW
		}
		puts $fp $row
	}
	close $fp
}
proc makeEffThickness { dir filename rows name1 cols name2 val name3 th th_name } {

	set datafile [file join $dir $filename ]
	set fp [open $datafile "w"]

	upvar $rows R
	upvar $cols C
	upvar $val  V

	printArray1df $fp R $name1
	printArray1df $fp C $name2

	puts $fp "$name3 for $th_name $th"

	for { set i 0 } { $i < [array size R] } { incr i } {
		set w $R($i)

		variable row ""
		for { set j 0 } { $j < [array size C] } { incr j } {
			set s $C($j)
        		set DWC $V($i,$j)

        		#puts "i= $i  j= $j  w= $w  s= $s   dwc= $DWC"

			set cW [ expr $th - $DWC ]
			lappend row $cW
		}
		puts $fp $row
	}
	close $fp
}
proc makeEffWidth_low { dir filename rows name1 cols name2 val1d val name3 th } {

	set datafile [file join $dir $filename ]
	set fp [open $datafile "w"]

	upvar $rows R
	upvar $val1d A
	upvar $cols C
	upvar $val  V

	printArray1df $fp R $name1
	printArray1df $fp C $name2

	puts $fp $name3

	for { set i 0 } { $i < [array size R] } { incr i } {
		set w $R($i)
		set a $A($i)

		variable row ""
		for { set j 0 } { $j < [array size C] } { incr j } {
			set s $C($j)
        		set DWC $V($i,$j)

        		#puts "i= $i  j= $j  w= $w  s= $s   dwc= $DWC"

#davidgu 1/3/06		set cW [ expr $w -2 * $a * ($th - $DWC) ]
			set cW [ expr 2 * $a * ($th - $DWC) ]
			lappend row $cW
		}
		puts $fp $row
	}
	close $fp
}


proc 1darray { name data } {
    upvar $name arr
    set idx 0
    set cnt [scan $data "%s %\[^\\]" num rem]
    while { $cnt > 1 } { 
        if { [string is double $num] } {
            set arr($idx) $num
            incr idx
        }
        set cnt [scan $rem "%s %\[^\\]" num rem]
    }
    if { [string is double $rem] } {
        set arr($idx) $rem
    }
}

proc 2darray { name row darr } {
    upvar 1 $name 2da
    upvar 1 $darr 1da

    for { set i 0 } { $i < [array size 1da] } { incr i } {
        set 2da($row,$i) $1da($i)
    }
}
proc get2Darray { dir filename SPACE s_name WIDTH w_name A a_name } {
    
	upvar 1 $SPACE space
	upvar 1 $WIDTH width
	upvar 1 $A 2d_name

	set datafile [file join $dir $filename ]

	set fp [open $datafile "r"]
	set data [read $fp]
	close $fp
	
	set data [split $data "\n"]

	# read S
	set i 0
	set S_flag 0
	set W_flag 0
	set dwr_flag 0
	
	foreach arg $data {
    		if { [string trim $arg] == $s_name } {
			set S_flag 1
			continue
    		}
    		if { [string trim $arg] == $w_name } {
			set W_flag 1
			continue
    		}
    		if { [string trim $arg] == $a_name } {
			set dwr_flag 1
			continue
    		}
    		if { $S_flag > 0 && [string trim $arg] != "" } {
        		1darray space $arg
        		set S_flag 0
			continue
    		}
    		if { $W_flag > 0 && [string trim $arg] != "" } {
        		1darray width $arg
        		set W_flag 0
			continue
    		}
    		if { $dwr_flag>0 && [string trim $arg] != "" } {
        		1darray "a$i" $arg
        		incr i
    		}
	}
	if { ($dwr_flag>0) } {
		for { set j 0 } { $j < $i } {incr j} {
    			2darray 2d_name $j "a$j"
		}
		# parray 2d_name
	}
}

foreach DIR { "M1" "M2" } {
	get2Darray $DIR "dwc" dwc_S "S" dwc_W "W" dwc "dwc"
	get2Darray $DIR "dwr" dwr_S "S" dwr_W "W" dwr "dwr"

	get2Darray $DIR "dtc" dtc_D "Deff" dtc_W "W" dtc "dtc"
	get2Darray $DIR "dtr" dtr_D "Deff" dtr_W "W" dtr "dtr"

	get2Darray $DIR "alpha" a_W "W" a "a" aaa "aaa"

	get2Darray $DIR "p" p_W "W" p "p" ppp "ppp"
	get2Darray $DIR "x" x "X" we "we" th "thickness"

	makeEffP $DIR "p.out" p_W "Width" p "P"
	makeEffWidth $DIR "hi_cw.out"  dwc_W "Width" dwc_S "Spacing" dwc "hi_cWidth_eff"
	makeEffWidth $DIR "hi_rw.out"  dwr_W "Width" dwr_S "Spacing" dwr "hi_rWidth_eff"

	makeEffThickness $DIR "cth.out"  dtc_W "Width" dtc_D "Deff" dtc "c_thickness_eff" $th(0,0) "thickness"
	makeEffThickness $DIR "rth.out"  dtr_W "Width" dtr_D "Deff" dtr "r_thickness_eff" $th(0,0) "thickness"

	# width arrays for both alpha and dtc are assumed to match
	makeEffWidth_low $DIR "lo_cw.out"  dtc_W "Width" dtc_D "Deff" a dtc "lo_cWidth_delta" $th(0,0)
	makeEffWidth_low $DIR "lo_rw.out"  dtr_W "Width" dtr_D "Deff" a dtr "lo_rWidth_delta" $th(0,0)
}
