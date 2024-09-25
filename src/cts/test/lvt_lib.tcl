source "helpers.tcl"
source "cts-helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_liberty Nangate45/Nangate45_lvt.lib
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/Nangate45_lvt.lef

set block [make_array 300 200000 200000 150]

sta::db_network_defined

create_clock -period 5 clk

set_wire_rc -clock -layer metal5

clock_tree_synthesis -library NangateOpenCellLibrary_lvt 


