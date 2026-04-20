# repair_setup w/ GCD design
source "helpers.tcl"

proc capture_setup_ppa { } {
  set scene [sta::cmd_scene]
  set wns [sta::worst_slack_cmd max]
  if { $wns > 0.0 } {
    set wns 0.0
  }
  set tns [sta::total_negative_slack_cmd max]
  set area_um2 [expr { [rsz::design_area] * 1e12 }]
  lassign [sta::design_power $scene] _internal _switching _leakage total_power
  return [list $wns $tns $area_um2 $total_power]
}

proc report_setup_ppa_delta { before after } {
  lassign $before before_wns before_tns before_area before_power
  lassign $after after_wns after_tns after_area after_power

  set ppa_row_format "%-9s WNS=%8s   TNS=%8s   Area=%8.0f um^2   Power=% 12.6e W"
  set delta_row_format "%-9s WNS=%8s   TNS=%8s   Area=%+8.0f um^2   Power=%+12.6e W"

  puts [format \
    $ppa_row_format \
    "Pre-ECO:" \
    [sta::format_time $before_wns 3] \
    [sta::format_time $before_tns 3] \
    $before_area \
    $before_power]
  puts [format \
    $ppa_row_format \
    "Post-ECO:" \
    [sta::format_time $after_wns 3] \
    [sta::format_time $after_tns 3] \
    $after_area \
    $after_power]
  puts [format \
    $delta_row_format \
    "Delta:" \
    [sta::format_time [expr { $after_wns - $before_wns }] 3] \
    [sta::format_time [expr { $after_tns - $before_tns }] 3] \
    [expr { $after_area - $before_area }] \
    [expr { $after_power - $before_power }]]
}

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_placed.def

create_clock [get_ports clk] -name core_clock -period 0.5
set_max_fanout 100 [current_design]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

buffer_ports

set ppa_before [capture_setup_ppa]

repair_timing -setup

set ppa_after [capture_setup_ppa]
report_setup_ppa_delta $ppa_before $ppa_after

#puts "Wrong wires:"
##report_long_wires 10
#
#puts "Floting nets:"
#report_floating_nets -verbose
#
#puts "Area:"
#report_design_area
#
#report_check_types -max_slew -max_fanout -max_capacitance
#report_worst_slack -min
#report_worst_slack -max

puts "pass"
