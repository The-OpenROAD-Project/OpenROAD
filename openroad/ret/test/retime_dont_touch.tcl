# Registers that must stay are fenced: with the register of retime_basic
# marked don't-touch nothing moves, and the same with a don't-touch net on
# its path, a don't-touch gate reading it, the register fixed in place, in a
# group or in a region, or an SDC exception or a constraint on a pin or on
# the cell naming it, or a port delay measured from its clock pin.  A gate
# in a group, or with a multicycle through it, is pinned (3 pinned gates,
# not 2).  A path group that lists the registers as cells keeps them in
# place too, until -move_named_registers releases them: the last retime
# moves the register.
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

set_timing_derate -late 1.05 [get_cells r1]
retime
unset_timing_derate

set_input_delay 0.01 -clock clk -reference_pin [get_pins r1/CK] [get_ports a]
retime
unset_input_delay -clock clk [get_ports a]

set block [ord::get_db_block]
set r1 [$block findInst r1]
$r1 setPlacementStatus FIRM
retime
$r1 setPlacementStatus NONE

set group [odb::dbGroup_create $block domain]
$group addInst $r1
retime
$group removeInst $r1

set region [odb::dbRegion_create $block area]
$region addInst $r1
retime
$region removeInst $r1

set gate [$block findInst g1]
$group addInst $gate
retime
$group removeInst $gate

set_multicycle_path 2 -through [get_pins g2/ZN]
retime
unset_path_exceptions -through [get_pins g2/ZN]

group_path -name registers -to [all_registers]
retime
retime -move_named_registers
