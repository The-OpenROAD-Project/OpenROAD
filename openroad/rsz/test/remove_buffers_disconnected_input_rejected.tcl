# A buffer with a disconnected input must remain unchanged after rejection.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def remove_buffers3.def

set block [ord::get_db_block]
set b2 [$block findInst b2]
set input_net [$block findNet n1]
set output_net [$block findNet n2]
[$b2 findITerm A] disconnect

# Given an undriven protected buffer, when removal is rejected, then its state persists.
$b2 setPlacementStatus FIRM
set_dont_touch b2
set_dont_touch n1
set_dont_touch n2
remove_buffers b2

puts "b2 input net: [[$b2 findITerm A] getNet]"
puts "b2 remains: [expr { [$block findInst b2] ne "NULL" }]"
puts "b2 dont_touch: [$b2 isDoNotTouch]"
puts "b2 placement: [$b2 getPlacementStatus]"
puts "n1 dont_touch: [$input_net isDoNotTouch]"
puts "n2 dont_touch: [$output_net isDoNotTouch]"
