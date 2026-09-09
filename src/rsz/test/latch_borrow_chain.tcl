source "helpers.tcl"

set test_name latch_borrow_chain

source Nangate45/Nangate45.vars
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def latch_borrow_chain.def

create_clock -name clk -period 1.0 clk
create_clock -name vclk -period 1.0
set_input_delay -clock vclk 0.84 [get_ports en_in]

source Nangate45/Nangate45.rc
source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk

set_propagated_clock [all_clocks]
estimate_parasitics -placement

proc read_report { report_file } {
  set stream [open $report_file r]
  set report_text [read $stream]
  close $stream
  return $report_text
}

proc extract_borrow { report_text label } {
  if { ![regexp {actual time borrow[ ]+([0-9.]+)} $report_text -> borrow] } {
    puts "Missing actual time borrow in $label report_checks output."
    return 0.0
  }
  return $borrow
}

proc extract_slack { report_text label } {
  foreach line [split $report_text "\n"] {
    if { [regexp {^[ ]*([-+]?[0-9.]+)[ ]+slack} $line -> slack] } {
      return $slack
    }
  }
  puts "Missing slack in $label report_checks output."
  return 0.0
}

proc timing_report { test_name label endpoint } {
  set report_file [make_result_file "${test_name}_${label}.rpt"]
  report_checks -path_delay max -to $endpoint \
    -group_path_count 1 -format full_clock_expanded -digits 4 > $report_file
  return [read_report $report_file]
}

set before_l1_report [timing_report $test_name "before_l1" "enable_latch/D"]
set before_l2_report [timing_report $test_name "before_l2" "deep_latch/D"]
set before_endpoint_report [timing_report $test_name "before_endpoint" "gated_ff0/D"]

set before_borrow_l1 [extract_borrow $before_l1_report "before_l1"]
set before_borrow_l2 [extract_borrow $before_l2_report "before_l2"]
set before_slack [extract_slack $before_endpoint_report "before_endpoint"]

puts [format "BEFORE_BORROW_L1 %.4f" $before_borrow_l1]
puts [format "BEFORE_BORROW_L2 %.4f" $before_borrow_l2]
puts [format "BEFORE_SLACK %.4f" $before_slack]

repair_timing -setup -skip_last_gasp -skip_pin_swap -skip_gate_cloning \
  -max_passes 20

set after_l1_report [timing_report $test_name "after_l1" "enable_latch/D"]
set after_l2_report [timing_report $test_name "after_l2" "deep_latch/D"]
set after_endpoint_report [timing_report $test_name "after_endpoint" "gated_ff0/D"]

set after_borrow_l1 [extract_borrow $after_l1_report "after_l1"]
set after_borrow_l2 [extract_borrow $after_l2_report "after_l2"]
set after_slack [extract_slack $after_endpoint_report "after_endpoint"]

puts [format "AFTER_BORROW_L1 %.4f" $after_borrow_l1]
puts [format "AFTER_BORROW_L2 %.4f" $after_borrow_l2]
puts [format "AFTER_SLACK %.4f" $after_slack]

if { !($after_slack > $before_slack + 0.001) } {
  puts [format \
    "repair_timing did not improve endpoint slack: before %.4f after %.4f" \
    $before_slack $after_slack]
}
