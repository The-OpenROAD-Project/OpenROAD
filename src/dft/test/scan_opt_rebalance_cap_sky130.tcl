# scan_opt -rebalance_chains keeps every chain within the longest chain's bits
# when the chains' last cells have different bit counts.
source "helpers.tcl"
source "scan_opt_helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_verilog scan_architect_sky130.v
link_design scan_architect

create_clock -name clock1 -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock1}]
create_clock -name clock2 -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock2}]

set_dft_config -max_length 3 -clock_mixing no_mix

scan_replace
scatter_insts 0
execute_dft_plan

# Give the chains unequal bit counts: in alternate chains the last cell
# carries 3 bits, and in the others every cell but the last carries 3.
set names {}
foreach chain [[[ord::get_db_block] getDft] getScanChains] {
  lappend names [$chain getName]
}
set i 0
foreach name [lsort -dictionary $names] {
  set cells [scan_chain_cells [scan_chain_by_name $name]]
  if { $i % 2 == 0 } {
    [lindex $cells end] setBits 3
  } else {
    foreach scan_inst [lrange $cells 0 end-1] {
      $scan_inst setBits 3
    }
  }
  incr i
}

proc max_chain_bits { } {
  set max 0
  foreach chain [[[ord::get_db_block] getDft] getScanChains] {
    set bits 0
    foreach scan_inst [scan_chain_cells $chain] {
      incr bits [$scan_inst getBits]
    }
    set max [expr { max($max, $bits) }]
  }
  return $max
}

set max_before [max_chain_bits]
set lasts [scan_chain_last_cells]
scatter_insts 5
scan_opt -rebalance_chains
check_scan_chains $lasts
puts "max chain bits: before $max_before after [max_chain_bits]"
