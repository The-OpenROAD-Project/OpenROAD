# odb don't-touch is honored: with the register of retime_basic marked
# don't-touch nothing moves, and the same with a don't-touch net on its
# path.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_basic.v
link_design top
create_clock -period 1.0 clk

set_dont_touch r1
retime
unset_dont_touch r1

set_dont_touch n2
retime
unset_dont_touch n2

retime
