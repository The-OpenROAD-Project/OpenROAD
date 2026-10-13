# Buffer-removal preflight must agree with the merge when both nets have ports.
source "helpers.tcl"
# State is asserted below, so golden output contains only the check summary.
suppress_message ODB 227
suppress_message RSZ 26
suppress_message RSZ 97

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog remove_buffers_survivor_mismatch.v
link_design top

set block [ord::get_db_block]
set b1 [$block findInst b1]
set u1 [$block findInst u1]
set input_net [[$block findBTerm in1] getNet]
set output_net [[$block findBTerm out1] getNet]
set other_input_net [[$block findBTerm in2] getNet]
check "candidate exists" { expr { $b1 ne "NULL" } } 1
check "protected sink exists" { expr { $u1 ne "NULL" } } 1
check "buffer input net" { [$b1 findITerm A] getNet } $input_net
check "buffer output net" { [$b1 findITerm Z] getNet } $output_net
check "sink output net" { [$u1 findITerm A] getNet } $output_net

set_dont_touch u1
remove_buffers b1
check "selected candidate preserved" { expr { [$block findInst b1] ne "NULL" } } 1
check "selected sink remains protected" { $u1 isDoNotTouch } 1
check "selected buffer input unchanged" { [$b1 findITerm A] getNet } $input_net
check "selected buffer output unchanged" { [$b1 findITerm Z] getNet } $output_net
check "selected sink connection unchanged" { [$u1 findITerm A] getNet } $output_net

# A disconnected output is valid and must not trigger a merge.
remove_buffers dangling_selected
check "selected dangling buffer removed" {
  expr { [$block findInst dangling_selected] eq "NULL" }
} 1
check "dangling input net unchanged" { [$block findBTerm in2] getNet } $other_input_net

remove_buffers
check "automatic candidate preserved" { expr { [$block findInst b1] ne "NULL" } } 1
check "automatic sink still protected" { $u1 isDoNotTouch } 1
check "automatic sink connection unchanged" { [$u1 findITerm A] getNet } $output_net
check "automatic dangling buffer removed" {
  expr { [$block findInst dangling_auto] eq "NULL" }
} 1

# With the sink unprotected, the buffer and both ports can merge.
unset_dont_touch u1
remove_buffers b1
check "unprotected candidate removed" { expr { [$block findInst b1] eq "NULL" } } 1
check "merged port net" { [$block findBTerm out1] getNet } $input_net
check "remaining sink connected" { [$u1 findITerm A] getNet } $input_net

exit_summary
