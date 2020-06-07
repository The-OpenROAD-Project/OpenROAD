proc write_tie_hi_fanout_def { filename tie_port load_port fanout } {
  set stream [open $filename "w"]

  set tie_port1 [get_lib_pin $tie_port]
  set tie_cell_name [get_property [get_property $tie_port1 lib_cell] name]
  set tie_port_name [get_property $tie_port1 name]

  set load_port1 [get_lib_pin $load_port]
  set load_cell_name [get_property [get_property $load_port1 lib_cell] name]
  set load_port_name [get_property $load_port1 name]

  puts $stream {VERSION 5.8 ;
NAMESCASESENSITIVE ON ;
DIVIDERCHAR "/" ;
BUSBITCHARS "[]" ;
DESIGN top ;
UNITS DISTANCE MICRONS 1000 ;}
  puts $stream "COMPONENTS [expr $fanout + 1] ;
    - t1 $tie_cell_name ;"
  set rows [expr int(sqrt($fanout))]
  # 10micron x/y spacing
  set spacing 10000
  for { set i 0 } { $i < $fanout } { incr i } {
    set x [expr ($i % $rows) * $spacing * 4]
    set y [expr int($i / $rows) * $spacing]
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

proc check_ties { tie_cell_name } {
  set tie_insts [get_cells -filter "ref_name == $tie_cell_name"]
  foreach tie_inst $tie_insts {
    set tie_pin [get_pins -of $tie_inst -filter "direction == output"]
    set net [get_nets -of $tie_inst]
    set pins [get_pins -of $net]
    if { [llength $pins] != 2 } {
      puts "Warning: tie inst [get_full_name $tie_inst] has [llength $pins] connections."
    }
  }
}
