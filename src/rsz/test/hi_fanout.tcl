# write_hi_fanout def

set header {VERSION 5.8 ; 
DIVIDERCHAR "/" ;
BUSBITCHARS "[]" ;

DESIGN hi_fanout ;

UNITS DISTANCE MICRONS 1000 ;

DIEAREA ( 0 0 ) ( 200000 200000 ) ;
}

set middle {
PINS 1 ;
- clk1 + NET clk1 + DIRECTION INPUT + USE SIGNAL 
  + LAYER metal1 ( 0 0 ) ( 100 100 ) + FIXED ( 1000 1000 ) N ;
END PINS

SPECIALNETS 2 ;
- VSS  ( * VSS )
  + USE GROUND ;
- VDD  ( * VDD )
  + USE POWER ;
END SPECIALNETS
}

# r1/Q -> r2/D r3/D .... r<fanout+2>/D
proc write_hi_fanout_def { filename fanout } {
  write_hi_fanout_def1 $filename $fanout \
    "drvr" "DFF_X1" "CK" "Q" \
    "load" "DFF_X1" "CK" "D"
}

# drvr_inst/drvr_port -> load0/D rload1/load_in .... load<fanout>/load_in
proc write_hi_fanout_def1 { filename fanout
                            drvr_inst drvr_cell drvr_clk drvr_out
                            load_inst load_cell load_clk load_in } {
  global header middle

  set stream [open $filename "w"]
  puts $stream $header
  puts $stream "COMPONENTS [expr $fanout + 1] ;"
  puts $stream "- $drvr_inst $drvr_cell + PLACED ( 1000 1000 ) N ;"
  set space 5000
  set i 0
  while {$i < $fanout} {
    puts $stream "- $load_inst$i $load_cell + PLACED ( [expr ($i % 10) * $space] [expr ($i / 10) *$space] ) N ;"
    incr i
  }
  puts $stream "END COMPONENTS"

  puts $stream $middle

  puts $stream "NETS 2 ;"
  puts $stream "- clk1 ( PIN clk1 )"
  if { $drvr_clk != "" } {
    puts -nonewline $stream " ( $drvr_inst $drvr_clk )"
  }
  if { $load_clk != "" } {
    set i 0
    while {$i < $fanout} {
      puts -nonewline $stream " ( $load_inst$i $load_clk )"
      if { [expr $i % 10] == 0 } {
        puts $stream ""
      }
      incr i
    }
    puts $stream " ;"
  }

  puts $stream "- net0 ( $drvr_inst $drvr_out )"
  set i 0
  while { $i < $fanout } {
    puts -nonewline $stream " ( $load_inst$i $load_in )"
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
