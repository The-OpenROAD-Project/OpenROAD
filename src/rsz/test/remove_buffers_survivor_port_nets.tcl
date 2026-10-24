# Buffer removal must check the same net it merges when both sides have ports.
source "helpers.tcl"
set test_name remove_buffers_survivor_port_nets

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog remove_buffers_survivor_mismatch.v
link_design top

set block [ord::get_db_block]
set u1 [$block findInst u1]

puts "Protected sink: selected removal"
set_dont_touch u1
remove_buffers b1
puts "b1 remains: [expr { [$block findInst b1] ne "NULL" }]"
puts "u1 dont_touch: [$u1 isDoNotTouch]"
puts "u1 input net: [[[$u1 findITerm A] getNet] getName]"
puts "in1 net: [[[$block findBTerm in1] getNet] getName]"
puts "out1 net: [[[$block findBTerm out1] getNet] getName]"

puts "Disconnected output: selected removal"
remove_buffers dangling_selected
puts "dangling_selected remains: [expr { [$block findInst dangling_selected] ne "NULL" }]"
puts "in2 net: [[[$block findBTerm in2] getNet] getName]"

puts "Protected sink and disconnected output: automatic removal"
remove_buffers
puts "b1 remains: [expr { [$block findInst b1] ne "NULL" }]"
puts "dangling_auto remains: [expr { [$block findInst dangling_auto] ne "NULL" }]"
puts "u1 dont_touch: [$u1 isDoNotTouch]"
puts "u1 input net: [[[$u1 findITerm A] getNet] getName]"

puts "Unprotected sink: selected removal"
unset_dont_touch u1
remove_buffers b1
puts "b1 remains: [expr { [$block findInst b1] ne "NULL" }]"
puts "u1 dont_touch: [$u1 isDoNotTouch]"
puts "u1 input net: [[[$u1 findITerm A] getNet] getName]"
puts "in1 net: [[[$block findBTerm in1] getNet] getName]"
puts "out1 net: [[[$block findBTerm out1] getNet] getName]"
