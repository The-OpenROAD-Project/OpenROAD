# Test scan_opt: compare routed wirelength with and without optimization
# Run with: -D USE_SCAN_OPT=1 to enable optimizer, omit for baseline
source "helpers.tcl"
source "flow_helpers.tcl"
source "sky130hd/sky130hd.vars"

read_libraries
read_verilog scan_opt_sky130hd.v
link_design scan_opt_test

create_clock -name clk -period 2.0 -waveform {0.0 1.0} [get_ports {clk}]

# Floorplan
initialize_floorplan -site $site \
  -die_area {0 0 300 300} \
  -core_area {10 10 290 290}

source $tracks_file

eval tapcell $tapcell_args

source $pdn_cfg
pdngen

# DFT
set_dft_config -max_length 200
scan_replace
execute_dft_plan

# IO + Placement
place_pins -hor_layers $io_placer_hor_layer -ver_layers $io_placer_ver_layer

foreach layer_adjustment $global_routing_layer_adjustments {
  lassign $layer_adjustment layer adjustment
  set_global_routing_layer_adjustment $layer $adjustment
}
set_routing_layers -signal $global_routing_layers \
  -clock $global_routing_clock_layers

global_placement -density $global_place_density \
  -pad_left $global_place_pad -pad_right $global_place_pad

set_placement_padding -global -left $detail_place_pad -right $detail_place_pad
detailed_placement

# Dump scan chain order (cell name, x, y) by following SI->SO nets
proc dump_scan_chain { filename } {
  set block [ord::get_db_block]
  set si_net_to_inst [dict create]
  set inst_info [dict create]
  foreach inst [$block getInsts] {
    set name [$inst getName]
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
      dict set si_net_to_inst $si_net $name
      set origin [$inst getOrigin]
      dict set inst_info $name [list [lindex $origin 0] [lindex $origin 1] $so_net]
    }
  }

  set fp [open $filename w]
  puts $fp "name,x,y"
  foreach bterm [$block getBTerms] {
    if {![string match "scan_in*" [$bterm getName]]} { continue }
    set net [$bterm getNet]
    if {$net eq "NULL"} { continue }
    set net_name [$net getName]
    if {![dict exists $si_net_to_inst $net_name]} { continue }
    set cur [dict get $si_net_to_inst $net_name]
    while {$cur ne ""} {
      set info [dict get $inst_info $cur]
      puts $fp "$cur,[lindex $info 0],[lindex $info 1]"
      set so_net [lindex $info 2]
      if {$so_net ne "" && [dict exists $si_net_to_inst $so_net]} {
        set cur [dict get $si_net_to_inst $so_net]
      } else {
        set cur ""
      }
    }
    break
  }
  close $fp
}

# Optionally optimize scan chain
if {[info exists ::env(USE_SCAN_OPT)] && $::env(USE_SCAN_OPT)} {
  puts "=== Running scan_opt ==="
  scan_opt
  dump_scan_chain [make_result_file scan_chain_after.csv]
} else {
  puts "=== Skipping scan_opt (baseline) ==="
  dump_scan_chain [make_result_file scan_chain_before.csv]
}

# Route
pin_access

set route_guide [make_result_file scan_opt.route_guide]
global_route -guide_file $route_guide \
  -congestion_iterations 100 -verbose

detailed_route -output_drc [make_result_file "scan_opt_drc.rpt"] \
  -output_maze [make_result_file "scan_opt_maze.log"] \
  -no_pin_access \
  -verbose 0

# Report total wirelength
puts "=== Total routed wirelength ==="
report_wire_length -summary -detailed_route

set db_file [make_result_file scan_opt_sky130hd.odb]
write_db $db_file

set verilog_file [make_result_file scan_opt_sky130hd.v]
write_verilog $verilog_file

puts "pass"
