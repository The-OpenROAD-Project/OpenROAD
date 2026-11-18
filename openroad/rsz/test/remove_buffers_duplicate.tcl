# Repeated instance names must remove each buffer once, not crash.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def remove_buffers1.def

set block [ord::get_db_block]
remove_buffers b1 b1
puts "instances: [llength [$block getInsts]]"
