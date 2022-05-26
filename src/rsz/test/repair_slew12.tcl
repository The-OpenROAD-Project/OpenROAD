# repair_design max_slew overlapping driver/load pins (degenerate steiner tree)
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_slew12.def

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# move u2/A on top of u1/Z 
set u1 [[ord::get_db_block] findInst "u1"]
set u1z [$u1 findITerm "Z"]
set u1a [$u1 findITerm "A"]
lassign [$u1z getAvgXY] ignore u1z_x u1z_y
lassign [$u1a getAvgXY] ignore u1a_x u1a_y
set u2 [[ord::get_db_block] findInst "u2"]
lassign [$u1 getOrigin] u1x u1y
$u2 setOrigin [expr $u1x + $u1z_x - $u1a_x] [expr $u1y + $u1z_y - $u1a_y]

set u2a [$u2 findITerm "A"]
lassign [$u2a getAvgXY] u2a_x u2a_y


# force a slew violation on u1/Z
set_input_transition 10 in1
repair_design
