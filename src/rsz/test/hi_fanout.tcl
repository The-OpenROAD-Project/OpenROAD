# write_hi_fanout def

set header {VERSION 5.8 ; 
DIVIDERCHAR "/" ;
BUSBITCHARS "[]" ;

DESIGN hi_fanout ;
}

set special_nets {
SPECIALNETS 2 ;
- VSS  ( * VSS )
  + USE GROUND ;
- VDD  ( * VDD )
  + USE POWER ;
END SPECIALNETS
}

# nangate45 drvr/Q -> load0/D load1/D .... load<fanout-1>/D
proc write_hi_fanout_def { filename fanout } {
  write_hi_fanout_def1 $filename $fanout \
    "drvr" "DFF_X1" "CK" "Q" \
    "load" "DFF_X1" "CK" "D" 5000 \
    "metal1" 1000
}

# drvr_inst/drvr_out -> load0/load_in rload1/load_in .... load<fanout>/load_in
proc write_hi_fanout_def1 { filename fanout
                            drvr_inst drvr_cell drvr_clk drvr_out
                            load_inst load_cell load_clk load_in load_space
                            port_layer dbu } {
  global header special_nets

  set stream [open $filename "w"]
  puts $stream $header
  puts $stream "UNITS DISTANCE MICRONS $dbu ;"
  write_diearea $stream $fanout $load_space

  puts $stream "COMPONENTS [expr $fanout + 1] ;"
  puts $stream "- $drvr_inst $drvr_cell + PLACED ( 1000 1000 ) N ;"
  write_fanout_loads $stream $fanout $load_inst $load_cell $load_in $load_space
  puts $stream "END COMPONENTS"

  puts $stream "PINS 1 ;"
  write_fanout_port $stream "clk1" $port_layer
  puts $stream "END PINS"

  puts $stream $special_nets

  puts $stream "NETS 2 ;"
  puts -nonewline $stream "- clk1 ( PIN clk1 )"
  if { $drvr_clk != "" } {
    puts -nonewline $stream " ( $drvr_inst $drvr_clk )"
  }
  write_fanout_clk_terms $stream $fanout $load_inst $load_clk
  puts $stream " ;"

  puts $stream "- net0 ( $drvr_inst $drvr_out )"
  write_fanout_load_terms $stream $fanout $load_inst $load_in
  puts $stream " ;"

  puts $stream "END NETS"

  puts $stream "END DESIGN"
  close $stream
}

# nangate45 reset -> load0/RN load1/RN .... load<fanout-1>/RN
proc write_hi_fanout_input_def { filename fanout } {
  write_hi_fanout_def2 $filename $fanout reset \
    "r" "DFFRS_X2" "CK" "RN" 5000 \
    "metal1" 1000
}

# in_port -> load0/load_in rload1/load_in .... load<fanout>/load_in
proc write_hi_fanout_def2 { filename fanout
                            in_port
                            load_inst load_cell load_clk load_in load_space
                            port_layer dbu } {
  global header special_nets

  set stream [open $filename "w"]
  puts $stream $header
  puts $stream "UNITS DISTANCE MICRONS $dbu ;"
  write_diearea $stream $fanout $load_space

  puts $stream "COMPONENTS $fanout ;"
  write_fanout_loads $stream $fanout $load_inst $load_cell $load_in $load_space
  puts $stream "END COMPONENTS"

  puts $stream "PINS 2 ;"
  write_fanout_port $stream $in_port $port_layer
  write_fanout_port $stream "clk1" $port_layer
  puts $stream "END PINS"

  puts $stream $special_nets

  puts $stream "NETS 2 ;"
  puts $stream "- $in_port ( PIN $in_port )"
  write_fanout_load_terms $stream $fanout $load_inst $load_in
  puts $stream " ;"
  puts -nonewline $stream "- clk1 ( PIN clk1 )"
  write_fanout_clk_terms $stream $fanout $load_inst $load_clk
  puts $stream " ;"

  puts $stream "END NETS"

  puts $stream "END DESIGN"
  close $stream
}

proc write_diearea { stream fanout load_space } {
  set row_count [row_count $fanout]
  set dx [expr ($row_count + 1) * $load_space]
  set dy [expr ($fanout / $row_count + 1) * $load_space]
  puts $stream "DIEAREA ( 0 0 ) ( $dx $dy ) ;"
}

proc row_count { fanout } {
  return [expr int(sqrt($fanout))]
}

proc write_fanout_port { stream port_name port_layer } {
  puts $stream "- $port_name + NET $port_name + DIRECTION INPUT + USE SIGNAL"
  puts $stream "+ LAYER $port_layer ( 0 0 ) ( 100 100 ) + FIXED ( 1000 1000 ) N ;"
}

proc write_fanout_loads { stream fanout load_inst load_cell load_in load_space } {
  set row_count [row_count $fanout]
  set i 0
  while {$i < $fanout} {
    puts $stream "- $load_inst$i $load_cell + PLACED ( [expr ($i % $row_count) * $load_space] [expr ($i / $row_count) * $load_space] ) N ;"
    incr i
  }
}

proc write_fanout_clk_terms { stream fanout load_inst load_clk } {
  if { $load_clk != "" } {
    set i 0
    while {$i < $fanout} {
      puts -nonewline $stream " ( $load_inst$i $load_clk )"
      if { [expr $i % 10] == 0 } {
        puts $stream ""
      }
      incr i
    }
  }
}

proc write_fanout_load_terms { stream fanout load_inst load_in } {
  set i 0
  while { $i < $fanout } {
    puts -nonewline $stream " ( $load_inst$i $load_in )"
    if { [expr $i % 10] == 0 } {
      puts $stream ""
    }
    incr i
  }
}
