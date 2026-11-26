# Registers that must stay are fenced: with the register of retime_basic
# marked don't-touch nothing moves, and the same with a don't-touch net on
# its path, a don't-touch gate reading it, the register fixed in place, or
# an SDC exception or a constraint on a pin or on the cell naming it.  A
# path group that lists the registers as cells does not fence them: the last
# retime moves the register.
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

set_disable_timing [get_pins r1/D]
retime
unset_disable_timing [get_pins r1/D]

set_timing_derate -late 1.0 [get_cells r1]
retime
unset_timing_derate

set r1 [[ord::get_db_block] findInst r1]
$r1 setPlacementStatus FIRM
retime
$r1 setPlacementStatus NONE

group_path -name registers -to [all_registers]
retime
