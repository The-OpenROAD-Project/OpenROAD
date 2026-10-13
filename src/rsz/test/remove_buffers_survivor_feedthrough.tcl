# A child input->output feedthrough requires retaining the input ModNet,
# even when only the output flat net carries a top-level port.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog remove_buffers_survivor_feedthrough.v
link_design -hier top

set block [ord::get_db_block]
set b1 [$block findInst u/b1]
set u1 [$block findInst u1]
if {$b1 eq "NULL" || $u1 eq "NULL"} {
  error "Missing hierarchical test buffer"
}
set output_net [[$u1 findITerm A] getNet]
set_dont_touch u1

remove_buffers u/b1
if {[$block findInst u/b1] eq "NULL" || ![$u1 isDoNotTouch]
    || [[$u1 findITerm A] getNet] ne $output_net} {
  error "Feedthrough removal bypassed dont_touch of the output-side sink"
}
