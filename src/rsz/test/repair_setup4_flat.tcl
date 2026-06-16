# repair_timing -setup 2 corners
source "helpers.tcl"
source Nangate45/Nangate45.vars
define_corners fast slow
read_liberty -corner slow Nangate45/Nangate45_slow.lib
read_liberty -corner fast Nangate45/Nangate45_fast.lib
read_lef Nangate45/Nangate45.lef
read_verilog repair_setup4_flat.v
link_design reg1

# Load the placed fixture to keep this test independent of floorplan and placement.
read_def -floorplan_initialize repair_setup4_flat.def


create_clock -period 0.3 clk


source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

report_worst_slack -max
report_tns -digits 3


repair_timing -setup -skip_last_gasp -skip_pin_swap -skip_gate_cloning \
  -skip_buffer_removal -max_passes 1

set verilog_file [make_result_file repair_setup4_flat_out.v]
write_verilog $verilog_file
diff_files $verilog_file repair_setup4_flat_out.vok
