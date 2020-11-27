# buffer_ports -input hi fanout reset net (no req time) -> max slew violation
source "helpers.tcl"

set header {VERSION 5.8 ; 
DIVIDERCHAR "/" ;
BUSBITCHARS "[]" ;

DESIGN hi_fanout ;

UNITS DISTANCE MICRONS 1000 ;

DIEAREA ( 0 0 ) ( 2000 2000 ) ;
}

set middle {
PINS 2 ;
- clk1 + NET clk1 + DIRECTION INPUT + USE SIGNAL 
  + LAYER metal1 ( 0 0 ) ( 100 100 ) + FIXED ( 1000 1000 ) N ;
- reset + NET reset + DIRECTION INPUT + USE SIGNAL 
  + LAYER metal1 ( 0 0 ) ( 100 100 ) + FIXED ( 1100 1100 ) N ;
END PINS

SPECIALNETS 2 ;
- VSS  ( * VSS )
  + USE GROUND ;
- VDD  ( * VDD )
  + USE POWER ;
END SPECIALNETS
}

proc write_hi_fanout_def { filename fanout } {
  global header middle

  set stream [open $filename "w"]
  puts $stream $header
  puts $stream "COMPONENTS $fanout ;"
  set space 5000
  set i 0
  while {$i < $fanout} {
    puts $stream "- r$i DFFRS_X2 + PLACED   ( [expr ($i % 10) * $space] [expr ($i / 10) *$space] ) N ;"
    incr i
  }
  puts $stream "END COMPONENTS"

  puts $stream $middle

  puts $stream "NETS 2 ;"
  puts $stream "- clk1 ( PIN clk1 )"
  set i 0
  while {$i < $fanout} {
    puts -nonewline $stream " ( r$i CK )"
    if { [expr $i % 10] == 0 } {
      puts $stream ""
    }
    incr i
  }
  puts $stream " ;"

  puts $stream "- reset ( PIN reset )"
  set i 0
  while { $i < $fanout } {
    puts -nonewline $stream " ( r$i RN )"
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

set def_filename [make_result_file "buffer_ports4.def"]
write_hi_fanout_def $def_filename 250

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def $def_filename
create_clock -period 1 clk1

estimate_parasitics -placement

buffer_ports -inputs

report_check_types -max_slew

repair_design

report_check_types -max_slew
