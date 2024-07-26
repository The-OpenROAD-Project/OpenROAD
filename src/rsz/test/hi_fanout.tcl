source "helpers.tcl"
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


proc write_output_port_unplaced { stream port_name } {
    puts $stream "- $port_name + NET $port_name + DIRECTION OUTPUT + USE SIGNAL ;"
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


proc write_clone_test_def { filename clone_gate fanout} {
    write_clone_test_def1 $filename $clone_gate $fanout \
	"drvr" "DFF_X1" "CK" "Q" "D" \
	"load" "DFF_X1" "CK" "D" 300000\
	"metal3" 1000
}

# drvr_inst/drvr_out -> load0/load_in rload1/load_in .... load<fanout>/load_in
proc write_clone_test_def1 { filename clone_gate fanout
                            drvr_inst drvr_cell drvr_clk drvr_out drvr_data
                            load_inst load_cell load_clk load_in load_spacing
                            port_layer dbu } {
    global header special_nets

    set stream [open $filename "w"]
    # Write out the DEF header and units (which are supplied)  
    puts $stream $header
    puts $stream "UNITS DISTANCE MICRONS $dbu ;"
    # Write out the die area 
    write_diearea $stream $fanout $load_spacing
    
    puts $stream "COMPONENTS [expr $fanout + 3] ;"
    puts $stream "- ${drvr_inst}_1 $drvr_cell + PLACED ( 1000 1000 ) N ;"
    puts $stream "- ${drvr_inst}_2 $drvr_cell + PLACED ( 1500 1500 ) N ;"
    puts $stream "- nand_inst_0 $clone_gate + PLACED ( 1250 1250 ) N ;"    
    
    # Add the gate here
    # Below routine should connect all the flop inputs and clocks to appropriate
    # nets 
    write_fanout_loads $stream $fanout $load_inst $load_cell $load_in $load_spacing
    puts $stream "END COMPONENTS"

    # Now write all the pins
    set count [expr 2 + $fanout]
    puts $stream "PINS $count ;"
    write_fanout_port $stream "clk1" $port_layer
    write_fanout_port $stream "data" $port_layer
    set i 0
    while {$i < $fanout} {
	set port_name output$i
	write_output_port_unplaced $stream $port_name
	incr i
    }    
    puts $stream "END PINS"
    #=============================================
    # Write out the VSS/VDD nets here
    puts $stream $special_nets

    # Connect the nets of the circuit
    # We have the three input nets, then we have the two nets that connect the DFF
    # output to the gate input
    # Lastly we have the gate output connected to all the flops.
    set net_count [expr 6 + $fanout]
    puts $stream "NETS $net_count ;"
    puts -nonewline $stream "- data ( PIN data )"
    if { $drvr_data != "" } {
	puts -nonewline $stream " ( ${drvr_inst}_1 $drvr_data )"
	puts -nonewline $stream " ( ${drvr_inst}_2 $drvr_data )"
    }
    puts $stream " ;"
    puts -nonewline $stream "- clk1 ( PIN clk1 )"
    if { $drvr_clk != "" } {
	puts -nonewline $stream " ( ${drvr_inst}_1 $drvr_clk )"
	puts -nonewline $stream " ( ${drvr_inst}_2 $drvr_clk )"
    }
    write_fanout_clk_terms $stream $fanout $load_inst $load_clk
    puts $stream " ;"

    # flops to the nand gate connections
    puts $stream "- clk_to_nand0 ( ${drvr_inst}_1 $drvr_out ) ( nand_inst_0 A1 ) ;"
    puts $stream "- clk_to_nand1 ( ${drvr_inst}_2 $drvr_out ) ( nand_inst_0 A2 ) ;"    
    # NAND to output flop connections 
    puts $stream "- net0 ( nand_inst_0 ZN )"
    write_fanout_load_terms $stream $fanout $load_inst $load_in
    puts $stream " ;"
    # Output flop connections
    set i 0
    while {$i < $fanout} {
	set net_name output$i
	set inst_name load$i
	puts $stream "- $net_name ( PIN $net_name ) ( $inst_name Q ) ;"
	incr i
    }        
    puts $stream "END NETS"    
    puts $stream "END DESIGN"
    close $stream
}
