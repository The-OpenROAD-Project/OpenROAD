# Registers that must stay are fenced: with the register of retime_basic
# marked don't-touch nothing moves, and the same with a don't-touch net on
# its path, a don't-touch gate reading it, or an SDC exception or constraint
# naming it.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_basic.v
link_design top
create_clock -period 0.1 clk

set_dont_touch r1
retime
unset_dont_touch r1

set_dont_touch n2
retime
unset_dont_touch n2

set_dont_touch g1
retime
unset_dont_touch g1

set_false_path -to [get_pins r1/D]
retime
unset_path_exceptions -to [get_pins r1/D]

set_clock_latency 0 [get_pins r1/CK]
retime
unset_clock_latency [get_pins r1/CK]

retime
