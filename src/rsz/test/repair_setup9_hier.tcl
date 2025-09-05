# check physical hierarchy of new buffer in hierarchical flow
source "helpers.tcl"
source Nangate45/Nangate45.vars

define_corners fast slow
read_liberty -corner slow Nangate45/Nangate45_slow.lib
read_liberty -corner fast Nangate45/Nangate45_fast.lib
read_lef Nangate45/Nangate45.lef
read_verilog repair_setup9_hier.v
link_design top -hier
read_def -floorplan_initialize repair_setup9_hier.def

#place the design
#initialize_floorplan -die_area "0 0 40 1200" -core_area "0 0 40 1200" \
#  -site FreePDK45_38x28_10R_NP_162NW_34O
#global_placement -skip_nesterov_place
#detailed_placement

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

repair_timing -setup -skip_last_gasp -skip_pin_swap -skip_gate_cloning \
  -skip_buffer_removal -max_passes 10

# generate .v and .def files
set verilog_filename "repair_setup9_hier_out.v"
set repaired_verilog_filename [make_result_file $verilog_filename]
write_verilog $repaired_verilog_filename
diff_file ${verilog_filename}ok $repaired_verilog_filename

set def_filename "repair_setup9_hier_out.def"
set repaired_def_filename [make_result_file $def_filename]
write_def $repaired_def_filename
diff_file ${def_filename}ok $repaired_def_filename

# TODO: there is an issue that the new buffer input u_mid1/u_leaf2/split2/A input is floating.

# TODO: the output .v and .def are not consistent

# TODO: no ROW statement in the output .def
