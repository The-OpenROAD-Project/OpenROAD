# Hierarchical feedthrough preflight must inspect the net actually merged.
source "helpers.tcl"
suppress_message ODB 227
suppress_message ORD 11
suppress_message RSZ 26
suppress_message RSZ 97

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog remove_buffers_survivor_feedthrough.v
link_design -hier top

set block [ord::get_db_block]
set b1 [$block findInst u/b1]
set u1 [$block findInst u1]
check "hierarchical buffer exists" { expr { $b1 ne "NULL" } } 1
check "hierarchical sink exists" { expr { $u1 ne "NULL" } } 1

set input_net [[$b1 findITerm A] getNet]
set output_net [[$u1 findITerm A] getNet]
set_dont_touch u1
remove_buffers u/b1
check "protected feedthrough preserved" {
  expr { [$block findInst u/b1] ne "NULL" }
} 1
check "protected hierarchical sink" { $u1 isDoNotTouch } 1
check "hierarchical input unchanged" { [$b1 findITerm A] getNet } $input_net
check "hierarchical sink unchanged" { [$u1 findITerm A] getNet } $output_net

unset_dont_touch u1
remove_buffers u/b1
check "unprotected feedthrough removed" {
  expr { [$block findInst u/b1] eq "NULL" }
} 1
check "port still connected" {
  expr { [[$block findBTerm out1] getNet] eq [[$u1 findITerm A] getNet] }
} 1

exit_summary
