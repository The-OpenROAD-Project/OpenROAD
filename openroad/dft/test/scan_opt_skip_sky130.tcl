# scan_opt leaves a chain alone when its last cell is falling-edge and earlier
# cells are rising-edge: falling-edge cells must come first, and the last cell
# drives scan-out, so no valid order exists.
source "helpers.tcl"
source "scan_opt_helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_verilog scan_architect_sky130.v
link_design scan_architect

create_clock -name clock1 -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock1}]
create_clock -name clock2 -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock2}]

set_dft_config -max_length 20 -clock_mixing clock_mix

scan_replace
scatter_insts 0
execute_dft_plan

# Alternate the clock edges along each chain and make the last cell
# falling-edge, so the rising-edge cells before it make the chain impossible
# to order. Grouping the cells by edge would change the order.
set before [dict create]
foreach chain [[[ord::get_db_block] getDft] getScanChains] {
  set i 0
  foreach scan_inst [scan_chain_cells $chain] {
    if { $i % 2 == 0 } {
      $scan_inst setClockEdge $::odb::dbScanInst_ClockEdge_Rising
    } else {
      $scan_inst setClockEdge $::odb::dbScanInst_ClockEdge_Falling
    }
    incr i
  }
  [lindex [scan_chain_cells $chain] end] setClockEdge \
    $::odb::dbScanInst_ClockEdge_Falling
  dict set before [$chain getName] [scan_chain_order $chain]
}

scatter_insts 11
scan_opt

foreach name [lsort -dictionary [dict keys $before]] {
  set chain [scan_chain_by_name $name]
  set same [expr { [scan_chain_order $chain] == [dict get $before $name] }]
  puts "$name: unchanged $same"
}
