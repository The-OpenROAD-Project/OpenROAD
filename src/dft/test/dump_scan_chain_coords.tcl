source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_verilog place_sort_sky130.v
link_design place_sort

create_clock -name main_clock -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock}]

set_dft_config -max_length 100

scan_replace

proc place_inst { inst x y } {
  set db_inst [[ord::get_db_block] findInst $inst]
  $db_inst setLocation $x $y
  $db_inst setOrient R0
  $db_inst setPlacementStatus PLACED
}

# Initial placement: simple diagonal for architect
place_inst ff1_clk1_rising  1000 1000
place_inst ff2_clk1_rising  2000 2000
place_inst ff3_clk1_rising  3000 3000
place_inst ff4_clk1_rising  4000 4000
place_inst ff5_clk1_rising  5000 5000
place_inst ff6_clk1_rising  6000 6000
place_inst ff7_clk1_rising  7000 7000
place_inst ff8_clk1_rising  8000 8000
place_inst ff9_clk1_rising  9000 9000
place_inst ff10_clk1_rising 10000 10000

execute_dft_plan

# After detailed placement: two-cluster layout
place_inst ff1_clk1_rising   1000  1000
place_inst ff2_clk1_rising  20000  2000
place_inst ff3_clk1_rising   2000  3000
place_inst ff4_clk1_rising  19000  4000
place_inst ff5_clk1_rising   3000  5000
place_inst ff6_clk1_rising  18000  6000
place_inst ff7_clk1_rising   4000  7000
place_inst ff8_clk1_rising  17000  8000
place_inst ff9_clk1_rising   5000  9000
place_inst ff10_clk1_rising 16000 10000

# Trace scan chain by following SI net connections.
# Start from the chain's scan_in BTerm and follow SO→SI nets.
proc scan_chain_wirelength { label } {
  set block [ord::get_db_block]
  # Map SI net name -> {x y SO_net_name} for chain following
  set si_net_to_cell [dict create]
  foreach inst [$block getInsts] {
    set name [$inst getName]
    if {![string match "*clk1_rising*" $name]} { continue }
    set origin [$inst getOrigin]
    set x [lindex $origin 0]
    set y [lindex $origin 1]
    set si_net ""
    set so_net ""
    foreach iterm [$inst getITerms] {
      set pname [[$iterm getMTerm] getName]
      if {$pname eq "SCD"} {
        set net [$iterm getNet]
        if {$net ne "NULL"} { set si_net [$net getName] }
      }
      if {$pname eq "Q"} {
        set net [$iterm getNet]
        if {$net ne "NULL"} { set so_net [$net getName] }
      }
    }
    if {$si_net ne ""} {
      dict set si_net_to_cell $si_net [list $x $y $so_net]
    }
  }

  # Follow chain from the scan_in BTerm
  set total_wl 0
  foreach bterm [$block getBTerms] {
    if {![string match "scan_in*" [$bterm getName]]} { continue }
    set net [$bterm getNet]
    if {$net eq "NULL"} { continue }
    set net_name [$net getName]
    if {![dict exists $si_net_to_cell $net_name]} { continue }
    set info [dict get $si_net_to_cell $net_name]
    set prev_x [lindex $info 0]
    set prev_y [lindex $info 1]
    set next_so [lindex $info 2]
    while {$next_so ne "" && [dict exists $si_net_to_cell $next_so]} {
      set info [dict get $si_net_to_cell $next_so]
      set cx [lindex $info 0]
      set cy [lindex $info 1]
      set total_wl [expr {$total_wl + abs($cx - $prev_x) + abs($cy - $prev_y)}]
      set prev_x $cx
      set prev_y $cy
      set next_so [lindex $info 2]
    }
    break
  }
  puts "${label} scan chain wirelength: $total_wl"
}

scan_chain_wirelength "BEFORE"

scan_opt

scan_chain_wirelength "AFTER"

exit
