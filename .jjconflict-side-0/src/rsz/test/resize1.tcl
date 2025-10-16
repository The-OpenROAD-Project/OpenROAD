# resize to target_slew
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def reg2.def

create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

set_load 30 r1q
rsz::resize_to_target_slew [get_pin r1/Q]
report_instance r1
