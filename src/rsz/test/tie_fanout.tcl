proc write_tie_hi_fanout_def { filename tie_port load_port fanout } {
  set stream [open $filename "w"]

  set tie_port1 [get_lib_pin $tie_port]
  set tie_cell_name [get_name [get_property $tie_port1 lib_cell]]
  set tie_port_name [get_name $tie_port1]

  set load_port1 [get_lib_pin $load_port]
  set load_cell_name [get_name [get_property $load_port1 lib_cell]]
  set load_port_name [get_name $load_port1]

  set rows [expr int(sqrt($fanout))]
  # 10micron x/y spacing
  set spacing 10000
  set x_origin 10000
  set y_origin 10000
  set size [expr $x_origin + $spacing * ($rows + 1)]

  puts $stream {VERSION 5.8 ;
DIVIDERCHAR "/" ;
BUSBITCHARS "[]" ;
DESIGN top ;
UNITS DISTANCE MICRONS 1000 ;
}
  puts $stream "DIEAREA ( 0 0 ) ( $size $size ) ;\n"
  puts $stream "COMPONENTS [expr $fanout + 1] ;
    - t1 $tie_cell_name ;"
  for { set i 0 } { $i < $fanout } { incr i } {
    set x [expr ($i % $rows) * $spacing + $x_origin]
    set y [expr int($i / $rows) * $spacing + $y_origin]
    puts $stream " - u$i BUF_X1 + PLACED ( $x $y ) N ;"
  }
  puts $stream "END COMPONENTS
NETS 1 ;"
  puts -nonewline $stream "- n1 ( t1 $tie_port_name ) "
  for { set i 0 } { $i < $fanout } { incr i } {
    puts -nonewline $stream "  ( u$i $load_port_name )"
    if { [expr $i % 5] == 0 } {
      puts $stream ""
    }
  }
  puts $stream " + USE SIGNAL ;"
  puts $stream "END NETS
END DESIGN"
  close $stream
}
