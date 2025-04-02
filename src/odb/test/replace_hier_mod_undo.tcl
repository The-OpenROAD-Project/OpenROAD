# Try to undo module swap 

source "helpers.tcl"
define_corners fast slow
read_liberty -corner slow Nangate45/Nangate45_slow.lib
read_liberty -corner fast Nangate45/Nangate45_fast.lib
read_lef Nangate45/Nangate45.lef

read_verilog replace_hier_mod1.v
link_design top -hier
create_clock -period 0.3 clk

#place the design
initialize_floorplan -die_area "0 0 40 1200"   -core_area "0 0 40 1200" -site FreePDK45_38x28_10R_NP_162NW_34O
global_placement -skip_nesterov_place
detailed_placement

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

puts "### Initial bc1 is buffer_chain ###"
report_cell_usage bc1
report_checks -through u1z -through r2/D

set db [ord::get_db]
if { $db eq "NULL" } {
  error "db is not defined"
}
set chip [$db getChip]
if { $chip eq "NULL" } {
  error "chip is not defined"
}
set block [$chip getBlock]
if { $block eq "NULL" } {
  error "block is not defined"
}
odb::dbDatabase_beginEco $block
puts "### swap bc1 to inv_chain ###"
#set_debug_level ODB replace_design 1
replace_design bc1 inv_chain
report_cell_usage bc1
estimate_parasitics -placement
report_checks -through u1z -through r2/D

puts "### swap bc1 back to buffer_chain ###"
#replace_design bc1 buffer_chain
odb::dbDatabase_endEco $block
odb::dbDatabase_undoEco $block
report_cell_usage bc1
estimate_parasitics -placement
report_checks -through u1z -through r2/D


