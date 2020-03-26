# write_hi_fanout verilog/def

set header {VERSION 5.5 ; 
NAMESCASESENSITIVE ON ;
DIVIDERCHAR "/" ;
BUSBITCHARS "[]" ;

DESIGN hi_fanout ;

UNITS DISTANCE MICRONS 1000 ;

DIEAREA ( 0 0 ) ( 2000 2000 ) ;
}

set middle {
PINS 1 ;
- clk1 + NET clk1 + DIRECTION INPUT + USE SIGNAL 
  + LAYER M1 ( 0 0 ) ( 100 100 ) + FIXED ( 1000 1000 ) N ;
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
  global header middle

  set stream [open $filename "w"]
  puts $stream $header
  puts $stream "COMPONENTS [expr $fanout + 1] ;"
  puts $stream "- r1 snl_ffqx1 + PLACED   ( 1000 1000 ) N ;"
  set space 5000
  set i 0
  while {$i < $fanout} {
    set r [expr $i + 2]
    puts $stream "- r$r snl_ffqx1 + PLACED   ( [expr ($r % 10) * $space] [expr ($r / 10) *$space] ) N ;"
    incr i
  }
  puts $stream "END COMPONENTS"

  puts $stream $middle

  puts $stream "NETS 2 ;"
  puts $stream "- clk1 ( PIN clk1 )"
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
