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

check "buffer remains" { expr { [$block findInst b2] ne "NULL" } } 1
check "buffer remains dont_touch" { $b2 isDoNotTouch } 1
check "buffer remains fixed" { $b2 getPlacementStatus } FIRM
check "input net remains dont_touch" { $input_net isDoNotTouch } 1
check "output net remains dont_touch" { $output_net isDoNotTouch } 1
exit_summary
