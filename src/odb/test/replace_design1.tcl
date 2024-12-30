# repair_timing -setup 2 corners
source "helpers.tcl"
define_corners fast slow
read_liberty -corner slow Nangate45/Nangate45_slow.lib
read_liberty -corner fast Nangate45/Nangate45_fast.lib
read_lef Nangate45/Nangate45.lef

read_verilog replace_design1.v
#read_def repair_setup1.def
link_design top -hier
create_clock -period 0.3 clk

#place the design
initialize_floorplan -die_area "0 0 40 1200"   -core_area "0 0 40 1200" -site FreePDK45_38x28_10R_NP_162NW_34O
global_placement -skip_nesterov_place
detailed_placement

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

report_checks -through u1z -through r2/D

write_verilog_for_eqy replace_design1 before "None"

replace_design bc1 inv_chain
estimate_parasitics -placement

run_equivalence_test replace_design1 ./Nangate45/work_around_yosys/ "None"

report_checks -through u1z -through r2/D

