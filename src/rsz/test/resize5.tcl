# resize reg Q/QN
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def resize5.def
create_clock -period 1 clk1

set_load  5 r1q
set_load 40 r1qn
rsz::resize
report_instance r1

# swap loads to check other resize order
set_load 40 r1q
set_load  5 r1qn
rsz::resize
report_instance r1
