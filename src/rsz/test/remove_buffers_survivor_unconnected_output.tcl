# The preflight must accept a buffer with no output net. removeBuffer()
# deletes it without attempting to merge a null net (OpenROAD #11722 review).
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog remove_buffers_survivor_unconnected_output.v
link_design top

set block [ord::get_db_block]
set b1 [$block findInst b1]
set b2 [$block findInst b2]
if {$b1 eq "NULL" || $b2 eq "NULL"} {
  error "Missing dangling buffer candidates"
}
if {[[$b1 findITerm Z] getNet] ne "NULL"
    || [[$b2 findITerm Z] getNet] ne "NULL"} {
  error "Test requires unconnected output pins"
}

set net1 [[$block findBTerm in1] getNet]
set net2 [[$block findBTerm in2] getNet]

# Exercise both explicit and automatic removal paths.
remove_buffers b1
if {[$block findInst b1] ne "NULL"} {
  error "Explicit removal rejected an unconnected output"
}
remove_buffers
if {[$block findInst b2] ne "NULL"} {
  error "Automatic removal rejected an unconnected output"
}
if {[[$block findBTerm in1] getNet] ne $net1
    || [[$block findBTerm in2] getNet] ne $net2} {
  error "Dangling-buffer removal changed input net connectivity"
}
