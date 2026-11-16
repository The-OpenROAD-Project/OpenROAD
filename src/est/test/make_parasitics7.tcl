# 2 corners with set_layer_rc
source "helpers.tcl"
read_liberty Nangate45/Nangate45_slow.lib
define_scene ss -liberty Nangate45/Nangate45_slow.lib
read_lef Nangate45/Nangate45.lef
read_def reg6.def

create_clock -period 10 clk
set_input_delay -clock clk 0 in1

# kohm/micron
set r 5.43e-3
# fF/micron
set c 6.013e-2

source Nangate45/Nangate45.rc
set_layer_rc -layer metal1 -scene ss \
  -resistance [expr { $r * 1.2 }] -capacitance [expr { $c * 1.2 }]
set_wire_rc -scene ss -layer metal1

set spef_file [make_result_file make_parasitics7.spef]

estimate_parasitics -placement -spef_file $spef_file

report_net r1/Q -scene ss

diff_files make_parasitics7.spefok $spef_file
