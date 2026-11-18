# retime on a design that has routing warns and leaves the routing as it is.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_basic.v
link_design top
create_clock -period 1.0 clk

set net [[ord::get_db_block] findNet clk]
set wire [odb::dbWire_create $net]
retime
puts "clk keeps its wire: [expr { [$net getWire] ne "NULL" }]"
