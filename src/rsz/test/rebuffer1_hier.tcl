# rebuffering test in hierarchical flow
source "helpers.tcl"

define_corners fast slow
read_liberty -corner slow Nangate45/Nangate45_slow.lib
read_liberty -corner fast Nangate45/Nangate45_fast.lib
read_lef Nangate45/Nangate45.lef
read_verilog rebuffer1_hier.v
link_design top -hier

#place the design
initialize_floorplan -die_area "0 0 40 1200" -core_area "0 0 40 1200" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
global_placement -skip_nesterov_place
detailed_placement

#sdc
create_clock -period 0.3 clk
set_clock_uncertainty -hold 0.1 [get_clocks clk]

#parasitic
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

report_worst_slack -max
report_worst_slack -min
report_tns -digits 3

#set_debug_level RSZ rebuffer 10

rsz::fully_rebuffer [get_pins u_mid1/u_leaf2/dff2/Q]

# generate .v and .def files
set rebuffered_verilog_filename "rebuffer1_hier_out.v"
write_verilog $rebuffered_verilog_filename
diff_file ${rebuffered_verilog_filename}ok $rebuffered_verilog_filename

set rebuffered_def_filename "rebuffer1_hier_out.def"
write_def $rebuffered_def_filename
diff_file ${rebuffered_def_filename}ok $rebuffered_def_filename

# TODO: there is an issue that the name name (\u_mid1/l2_out1) of 
#       the new buffer input u_mid1/u_leaf2/rebuffer1/A is wrong
