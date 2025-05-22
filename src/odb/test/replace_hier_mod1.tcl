# Test hier module swap and reverse swap

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

puts "### Initial bc1 is buffer_chain ###"
report_cell_usage bc1
report_net u1z -digits 3
report_net u3z -digits 3
estimate_parasitics -placement
report_checks -through u1z -through r2/D -digits 3

puts "### swap bc1 to inv_chain ###"
#set_debug_level ODB replace_design 1
replace_hier_module bc1 inv_chain
global_placement -skip_nesterov_place -incremental
detailed_placement
report_cell_usage bc1
report_net u1z -digits 3
report_net u3z -digits 3
estimate_parasitics -placement
report_checks -through u1z -through r2/D -digits 3

puts "### swap bc1 back to buffer_chain ###"
replace_hier_module bc1 buffer_chain
global_placement -skip_nesterov_place -incremental
detailed_placement
report_cell_usage bc1
report_net u1z -digits 3
report_net u3z -digits 3
estimate_parasitics -placement
report_checks -through u1z -through r2/D -digits 3

puts "### swap bc1 back to inv_chain ###"
replace_hier_module bc1 inv_chain
global_placement -skip_nesterov_place -incremental
detailed_placement
report_cell_usage bc1
report_net u1z -digits 3
report_net u3z -digits 3
estimate_parasitics -placement
report_checks -through u1z -through r2/D -digits 3

run_equivalence_test replace_hier_mod1 ./Nangate45/work_around_yosys/ "None"

