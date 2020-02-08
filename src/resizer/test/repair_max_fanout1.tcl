source helpers.tcl

# not used
proc write_hi_fanout_verilog { filename fanout } {
  set stream [open $filename "w"]
  puts $stream "module top (clk1, in1);"
  puts $stream " input clk1, in1;"
  puts $stream " snl_bufx1 u1 (.A(in1), .Z(u1z));"
  for {set i 0} {$i < $fanout} {incr i} {
    set reg_name "r$i"
    # constant value for sim updates
    puts $stream " snl_ffqx1 $reg_name (.CP(clk1), .D(u1z));"
  }
  puts $stream "endmodule"
  close $stream
}

proc write_hi_fanout_def { filename fanout } {
  set stream [open $filename "w"]
  puts $stream {VERSION 5.8 ;
NAMESCASESENSITIVE ON ;
DIVIDERCHAR "/" ;
BUSBITCHARS "[]" ;
DESIGN top ;
UNITS DISTANCE MICRONS 1000 ;}
  puts $stream "COMPONENTS [expr $fanout + 1] ;
 - u1 snl_bufx1 ;"
  set rows [expr int(sqrt($fanout))]
  # 10micron x/y spacing
  set spacing 10000
  for { set i 0 } { $i < $fanout } { incr i } {
    set x [expr ($i % $rows) * $spacing * 4]
    set y [expr int($i / $rows) * $spacing]
    puts $stream " - r$i snl_ffqx1 + PLACED ( $x $y ) N ;"
  }
  puts $stream "END COMPONENTS
PINS 2 ;
    - clk1 + NET clk1 + DIRECTION INPUT + USE SIGNAL ;
    - in1 + NET in1 + DIRECTION INPUT + USE SIGNAL ;
END PINS
NETS 3 ;"
  puts -nonewline $stream "- clk1 ( PIN clk1 ) "
  for { set i 0 } { $i < $fanout } { incr i } {
    puts -nonewline $stream "  ( r$i CP )"
    if { [expr $i % 5] == 0 } {
      puts $stream ""
    }
  }
  puts $stream " + USE SIGNAL ;"
  puts $stream " - in1 ( PIN in1 ) ( u1 A ) + USE SIGNAL ;"
  puts -nonewline $stream " - u1z ( u1 Z )"
  for { set i 0 } { $i < $fanout } { incr i } {
    puts -nonewline $stream "  ( r$i D )"
    if { [expr $i % 5] == 0 } {
      puts $stream ""
    }
  }
  puts $stream " + USE SIGNAL ;"
  puts $stream "END NETS
END DESIGN"
  close $stream
}

set def_filename [file join $result_dir "hi_fanout.def"]
write_hi_fanout_def $def_filename 35

read_liberty liberty1.lib
read_lef liberty1.lef
read_def $def_filename
create_clock -period 10 clk1

repair_max_fanout -max_fanout 10 -buffer_cell liberty1/snl_bufx1

foreach drvr {u1/Z buffer1/Z buffer2/Z buffer3/Z buffer4/Z} {
  set fanout [expr [llength [get_pins -of [get_net -of [get_pin $drvr]]]] - 1]
  puts "$drvr fanout $fanout"
  #set dist [sta::max_load_manhatten_distance [get_pin $drvr]]
  #puts "$drvr fanout $fanout dist [format %.0f [expr $dist * 1e6]]"
  #report_object_names [get_pins -of [get_net $ent]]
}

# set repaired_filename [file join $result_dir "repair_max_fanout1.def"]
# write_def $repaired_filename
# diff_file $repaired_filename repair_max_fanout1.defok
