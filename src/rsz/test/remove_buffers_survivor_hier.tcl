# A feedthrough must preserve its input ModNet and respect protected sinks.
source "helpers.tcl"
set test_name remove_buffers_survivor_hier

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog remove_buffers_survivor_feedthrough.v
link_design -hier top

set block [ord::get_db_block]
set u1 [$block findInst u1]

puts "Protected sink: hierarchical removal"
set_dont_touch u1
remove_buffers u/b1
puts "u/b1 remains: [expr { [$block findInst u/b1] ne "NULL" }]"
puts "u1 dont_touch: [$u1 isDoNotTouch]"
puts "u1 input net: [[[$u1 findITerm A] getNet] getName]"
puts "out1 net: [[[$block findBTerm out1] getNet] getName]"

puts "Unprotected sink: hierarchical removal"
unset_dont_touch u1
remove_buffers u/b1
puts "u/b1 remains: [expr { [$block findInst u/b1] ne "NULL" }]"
puts "u1 dont_touch: [$u1 isDoNotTouch]"
puts "u1 input net: [[[$u1 findITerm A] getNet] getName]"
puts "out1 net: [[[$block findBTerm out1] getNet] getName]"

# Include the serialized feedthrough assign in the normal golden log.
set out_verilog [make_result_file "${test_name}.v"]
write_verilog $out_verilog
report_file $out_verilog
