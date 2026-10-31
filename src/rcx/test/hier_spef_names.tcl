# Hierarchical db: write_spef must name objects that a tool adds later with a
# plain name inside a sub-module (like CTS clock buffers and clock nets) by
# their full path, as it does for names from read_verilog (u_sub/n1).
source helpers.tcl
source Nangate45/Nangate45.vars

read_lef $tech_lef
read_lef $std_cell_lef
read_liberty $liberty_file

read_verilog hier_spef_names.v
link_design top -hier

initialize_floorplan -site $site -utilization 10 -core_space 0.0

source $tracks_file
place_pins -hor_layers $io_placer_hor_layer -ver_layers $io_placer_ver_layer

set block [ord::get_db_block]

# Insert a buffer the way CTS does: plain names ("clkbuf_0", "clknet_0"),
# the instance owned by module sub, between u_sub/u_root and u_sub/ff.
set buf [odb::dbInst_create $block [[ord::get_db] findMaster BUF_X1] clkbuf_0]
[$block findModule sub] addInst $buf
set clknet [odb::dbNet_create $block clknet_0]
set ck [[$block findInst u_sub/ff] findITerm CK]
$ck disconnect
$ck connect $clknet
[$buf findITerm A] connect [$block findNet u_sub/n1]
[$buf findITerm Z] connect $clknet

# Place the instances
set x 0
foreach name {u_sub/u_root clkbuf_0 u_sub/ff} {
  set i [$block findInst $name]
  $i setLocation $x 0
  $i setPlacementStatus PLACED
  incr x 5700
}
detailed_placement

# Route the nets
set_routing_layers -signal $global_routing_layers
global_route
detailed_route -verbose 0

# Extract
define_process_corner -ext_model_index 0 X
set_extraction_rules_file $rcx_rules_file
extract_parasitics

set spef_file [make_result_file hier_spef_names.spef]
write_spef $spef_file

# The name map: clkbuf_0 and clknet_0 must read u_sub/clkbuf_0, u_sub/clknet_0
set f [open $spef_file]
set in_map 0
while { [gets $f line] >= 0 } {
  if { $line eq "*NAME_MAP" } {
    set in_map 1
  } elseif { $in_map && [string match "\\**" $line] && [llength $line] == 2 } {
    puts $line
  } elseif { $in_map && [string match "\\*PORTS*" $line] } {
    break
  }
}
close $f

# Test reading the spef for no errors in the .ok
read_spef $spef_file
