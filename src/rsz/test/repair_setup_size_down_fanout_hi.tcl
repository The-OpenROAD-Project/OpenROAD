# Regression for size_down_fanout's real purpose: asymmetric fanout
# criticality on a high-fanout net (16 loads, above the old hard fanout limit
# of 10).  One load (load0) sits on a tight, setup-critical output path; the
# other 15 fanouts have slack.  size_down_fanout should shed capacitance by
# downsizing only the slack-bearing loads (BUF_X8 -> BUF_X4) while preserving
# the critical load, whose delay budget is too tight to give up drive strength.
# The lighter high-fanout net speeds the shared driver, improving the critical
# path's slack even though the critical load itself is untouched.
source "helpers.tcl"

# in1 -> drvr/A, drvr/Z -> net0 (fanout loads), load<i>/Z -> out<i>.
proc write_size_down_hi_def { filename fanout drvr_cell load_cell } {
  set dbu 1000
  set space 5000
  set cols 4
  set rows [expr { ($fanout + $cols - 1) / $cols }]
  set dx [expr { ($cols + 2) * $space }]
  set dy [expr { ($rows + 2) * $space }]

  set stream [open $filename "w"]
  puts $stream "VERSION 5.8 ;"
  puts $stream "DIVIDERCHAR \"/\" ;"
  puts $stream "BUSBITCHARS \"\[\]\" ;"
  puts $stream "DESIGN size_down_fanout_hi ;"
  puts $stream "UNITS DISTANCE MICRONS $dbu ;"
  puts $stream "DIEAREA ( 0 0 ) ( $dx $dy ) ;"

  puts $stream "COMPONENTS [expr { $fanout + 1 }] ;"
  puts $stream "- drvr $drvr_cell + PLACED ( $space $space ) N ;"
  for { set i 0 } { $i < $fanout } { incr i } {
    set x [expr { (($i % $cols) + 1) * $space }]
    set y [expr { (($i / $cols) + 2) * $space }]
    puts $stream "- load$i $load_cell + PLACED ( $x $y ) N ;"
  }
  puts $stream "END COMPONENTS"

  puts $stream "PINS [expr { $fanout + 1 }] ;"
  puts $stream "- in1 + NET in1 + DIRECTION INPUT + USE SIGNAL"
  puts $stream "  + LAYER metal1 ( 0 0 ) ( 100 100 ) + FIXED ( 0 $space ) N ;"
  for { set i 0 } { $i < $fanout } { incr i } {
    puts $stream "- out$i + NET out$i + DIRECTION OUTPUT + USE SIGNAL ;"
  }
  puts $stream "END PINS"

  puts $stream "SPECIALNETS 2 ;"
  puts $stream "- VSS  ( * VSS )"
  puts $stream "  + USE GROUND ;"
  puts $stream "- VDD  ( * VDD )"
  puts $stream "  + USE POWER ;"
  puts $stream "END SPECIALNETS"

  puts $stream "NETS [expr { $fanout + 2 }] ;"
  puts $stream "- in1 ( PIN in1 ) ( drvr A ) ;"
  puts -nonewline $stream "- net0 ( drvr Z )"
  for { set i 0 } { $i < $fanout } { incr i } {
    puts -nonewline $stream " ( load$i A )"
    if { $i % 8 == 7 } {
      puts $stream ""
    }
  }
  puts $stream " ;"
  for { set i 0 } { $i < $fanout } { incr i } {
    puts $stream "- out$i ( PIN out$i ) ( load$i Z ) ;"
  }
  puts $stream "END NETS"

  puts $stream "END DESIGN"
  close $stream
}

proc report_load_cells { fanout } {
  puts "load cells:"
  for { set i 0 } { $i < $fanout } { incr i } {
    puts "  load$i [get_property [get_cells load$i] ref_name]"
  }
}

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

set def_file [make_result_file repair_setup_size_down_fanout_hi.def]
write_size_down_hi_def $def_file 16 BUF_X4 BUF_X8
read_def $def_file

# net0 has 16 loads + the driver pin, above the old hard fanout limit of 10.
puts "Loads on net0 (incl driver): [llength [get_pins -of_objects [get_nets net0]]]"

# Tight output delay only on out0 makes that fanout the setup-critical one;
# the other 15 outputs are timing-loose so their loads carry slack.  out0 also
# drives a heavy external load so load0 genuinely needs its BUF_X8 drive
# strength: downsizing load0 would blow its (near-zero) delay budget, so the
# move must preserve it while shrinking the 15 slack loads.
create_clock -name vclk -period 0.30
set_input_delay -clock vclk 0.05 [get_ports in1]
set_output_delay -clock vclk 0.0 [get_ports out*]
set_output_delay -clock vclk 0.22 [get_ports out0]
set_load 0.02 [all_outputs]
set_load 20 [get_ports out0]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement
set_dont_use [get_lib_cells CLKBUF*]

# Before: uniform BUF_X8 loads, heavy driver load, critical out0 violated.
report_worst_slack -max
report_checks -through out0 -fields {capacitance} -digits 3
report_load_cells 16

repair_timing -setup -sequence "size_down_fanout" -skip_last_gasp -max_passes 5

# After: the slack loads shrank to BUF_X4, load0 stayed BUF_X8, the driver load
# capacitance dropped and the critical out0 slack improved.
report_worst_slack -max
report_checks -through out0 -fields {capacitance} -digits 3
report_load_cells 16
