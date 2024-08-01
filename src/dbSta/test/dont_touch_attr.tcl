# port assigned to net (port with alias)
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog dont_touch_attr.v
link_design top

puts [[[ord::get_db_block] findInst u1] isDoNotTouch]
puts [[[ord::get_db_block] findInst u2] isDoNotTouch]
