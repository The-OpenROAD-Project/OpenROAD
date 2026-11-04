# dont_touch attribute values: bare, boolean strings, unrecognized
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog dont_touch_attr.v
link_design top

foreach name {u1 u2 u3 u4 u5 u6 u7 u8} {
  puts "$name [[[ord::get_db_block] findInst $name] isDoNotTouch]"
}
