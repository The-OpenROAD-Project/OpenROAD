# Control: the same port-to-port buffer remains removable without a
# protected sink. Both BTerms and the remaining sink must share one flat net.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog remove_buffers_survivor_mismatch.v
link_design top

set block [ord::get_db_block]
remove_buffers b1
if {[$block findInst b1] ne "NULL"} {
  error "Unprotected buffer was not removed"
}
set in_net [[$block findBTerm in1] getNet]
set out_net [[$block findBTerm out1] getNet]
set u1 [$block findInst u1]
if {$in_net eq "NULL" || $u1 eq "NULL" || $in_net ne $out_net
    || [[$u1 findITerm A] getNet] ne $in_net} {
  error "Port or sink connectivity changed after valid buffer removal"
}
