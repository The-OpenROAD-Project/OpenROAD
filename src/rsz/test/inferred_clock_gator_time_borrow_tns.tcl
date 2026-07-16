source "helpers.tcl"

set test_name inferred_clock_gator_time_borrow_tns

source Nangate45/Nangate45.vars
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def inferred_clock_gator_time_borrow.def

create_clock -name clk -period 1.0 clk
create_clock -name vclk -period 1.0
set_input_delay -clock vclk 0.98 [get_ports en_in]

source Nangate45/Nangate45.rc
source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk

set_propagated_clock [all_clocks]
estimate_parasitics -placement

proc extract_borrow { report_text } {
  if { ![regexp {actual time borrow[ ]+([0-9.]+)} $report_text -> borrow] } {
    puts "Missing actual time borrow in report_checks output."
  }
  return $borrow
}

proc borrow_report { test_name label } {
  set report_file [make_result_file "${test_name}_${label}.rpt"]
  report_checks -path_delay max -to enable_latch/D \
    -group_path_count 1 -format full_clock_expanded -digits 4 > $report_file
  set stream [open $report_file r]
  set report_text [read $stream]
  close $stream
  return $report_text
}

set before_report [borrow_report $test_name "before"]
set before_borrow [extract_borrow $before_report]
puts [format "BEFORE_BORROW %.4f" $before_borrow]

repair_timing -setup -phases "TNS" -skip_last_gasp -skip_pin_swap \
  -skip_gate_cloning -max_passes 20

set after_report [borrow_report $test_name "after"]
set after_borrow [extract_borrow $after_report]
puts [format "AFTER_BORROW %.4f" $after_borrow]

if { !($after_borrow < $before_borrow - 0.001) } {
  puts [format \
    "repair_timing did not reduce latch borrow: before %.4f after %.4f" \
    $before_borrow $after_borrow]
}
