# retime on a design that has routing warns and leaves the routing as it is,
# also on a data net whose loads it changes.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_basic.v
link_design top
create_clock -period 0.1 clk

set net [[ord::get_db_block] findNet clk]
odb::dbWire_create $net
set data [[ord::get_db_block] findNet n0]
odb::dbWire_create $data
retime
puts "clk keeps its wire: [expr { [$net getWire] ne "NULL" }]"
puts "n0 keeps its wire: [expr { [$data getWire] ne "NULL" }]"
