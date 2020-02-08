# rebuffer hi fanout register array

set header {VERSION 5.5 ; 
NAMESCASESENSITIVE ON ;
DIVIDERCHAR "/" ;
BUSBITCHARS "[]" ;

DESIGN reg1 ;
TECHNOLOGY technology ;

UNITS DISTANCE MICRONS 1000 ;

DIEAREA ( -1000 -1000 ) ( 1000 1000 ) ;
}

set middle {
PINS 1 ;
- clk + NET clk + DIRECTION INPUT + USE SIGNAL 
  + LAYER M1 ( -100 0 ) ( 100 1040 ) + FIXED ( 100000 100000 ) N ;
END PINS

SPECIALNETS 2 ;
- VSS  ( * VSS )
  + USE GROUND ;
- VDD  ( * VDD )
  + USE POWER ;
END SPECIALNETS
}

proc write_reg_fanout_def { fanout filename } {
  global header middle

  set stream [open $filename "w"]
  puts $stream $header
  puts $stream "COMPONENTS [expr $fanout + 1] ;"
  puts $stream "- r1 snl_ffqx1 + PLACED   ( 1000 1000 ) N ;"
  set space 5000
  set i 0
  while {$i < $fanout} {
    set r [expr $i + 2]
    puts $stream "- r$r snl_ffqx1 + PLACED   ( [expr ($r % 10) * $space] [expr ($r / 10) * $space] ) N ;"
    incr i
  }
  puts $stream "END COMPONENTS"

  puts $stream $middle

  puts $stream "NETS 2 ;"
  puts $stream "- clk ( PIN clk )"
  set i 0
  while {$i < [expr $fanout + 1]} {
    set r [expr $i + 1]
    puts -nonewline $stream " ( r$r CP )"
    if { [expr $i % 10] == 0 } {
      puts $stream ""
    }
    incr i
  }
  puts $stream " ;"

  puts $stream "- r1q ( r1 Q )"
  set i 0
  while { $i < $fanout } {
    set r [expr $i + 2]
    puts -nonewline $stream " ( r$r D )"
    if { [expr $i % 10] == 0 } {
      puts $stream ""
    }
    incr i
  }
  puts $stream " ;"

  puts $stream "END NETS"

  puts $stream "END DESIGN"
  close $stream
}

source "helpers.tcl"
read_liberty liberty1.lib
read_lef liberty1.lef
set def_file [make_result_file rebuffer3.def]
write_reg_fanout_def 40 $def_file
read_def $def_file
create_clock clk -period 1

# kohm/micron, pf/micron
# use 100x wire cap to tickle buffer insertion
set_wire_rc -resistance 1.7e-4 -capacitance 1.3e-2
report_worst_slack

set buffer_cell [get_lib_cell liberty1/snl_bufx2]
repair_max_cap -buffer_cell $buffer_cell
report_worst_slack
