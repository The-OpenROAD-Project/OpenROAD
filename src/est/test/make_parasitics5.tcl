# 2 corners with set_wire_rc
source "helpers.tcl"
read_liberty Nangate45/Nangate45_slow.lib
read_liberty Nangate45/Nangate45_fast.lib
define_scene ss -liberty Nangate45/Nangate45_slow.lib
define_scene ff -liberty Nangate45/Nangate45_fast.lib
set_scene ss
read_lef Nangate45/Nangate45.lef
read_def reg6.def

create_clock -period 10 clk
set_input_delay -clock clk 0 in1

# kohm/micron
set r 5.43e-3
# fF/micron
set c 6.013e-2

set_wire_rc -scene ff -resistance [expr { $r * 0.8 }] \
  -capacitance [expr { $c * 0.8 }]
set_wire_rc -scene ss -resistance [expr { $r * 1.2 }] \
  -capacitance [expr { $c * 1.2 }]
estimate_parasitics -placement

report_net r1/Q -scene ff
report_net r1/Q -scene ss
