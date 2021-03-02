# resize -dont_use
source helpers.tcl
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def reg2.def

create_clock -name clk -period 1 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}
# no placement, so add loads
set_load 50 r1q
set_load 50 r2q
set_load 50 u1z
set_load 50 u2z

report_checks -fields {slew input_pin} -digits 3
set_dont_use {*/*8 */*16}
rsz::resize
report_checks -fields {slew input_pin} -digits 3
