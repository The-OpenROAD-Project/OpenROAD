# Both adjacent nets have ports. A dont_touch output sink must prevent the
# removal of b1 because the output net, not the input net, is merged away.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog remove_buffers_survivor_mismatch.v
link_design top

set block [ord::get_db_block]
set b1 [$block findInst b1]
set u1 [$block findInst u1]
if {$b1 eq "NULL" || $u1 eq "NULL"} {
  error "Missing test buffer"
}
set input_net [[$block findBTerm in1] getNet]
set output_net [[$block findBTerm out1] getNet]
set_dont_touch u1

# The user-selected override must not override a protected *other* instance.
remove_buffers b1
if {[$block findInst b1] eq "NULL" || ![$u1 isDoNotTouch]} {
  error "Selected removal bypassed dont_touch of the output-side sink"
}
if {[[$b1 findITerm A] getNet] ne $input_net
    || [[$b1 findITerm Z] getNet] ne $output_net
    || [[$u1 findITerm A] getNet] ne $output_net} {
  error "Rejected buffer removal modified the nets"
}

# Automatic removal must reject the same candidate without changing nets.
remove_buffers
if {[$block findInst b1] eq "NULL" || ![$u1 isDoNotTouch]
    || [[$u1 findITerm A] getNet] ne $output_net} {
  error "Automatic removal bypassed dont_touch of the output-side sink"
}
