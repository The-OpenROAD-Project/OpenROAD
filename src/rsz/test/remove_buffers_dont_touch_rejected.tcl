# A rejected user-selected buffer removal must not clear its dont_touch flag.
# It must also retain fixed placement and adjacent net protection.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def remove_buffers3.def

# b3 prevents the merge when b2 is selected.  The override path may remove
# dont_touch after a successful eligibility check, but b2 must remain intact
# when the later merge check rejects it.
set block [ord::get_db_block]
set b2 [$block findInst b2]
set input_net [$block findNet n1]
set output_net [$block findNet n2]
$b2 setPlacementStatus FIRM
set_dont_touch b2
set_dont_touch b3
set_dont_touch n1
set_dont_touch n2
remove_buffers b2

if { $b2 == "NULL" || ![$b2 isDoNotTouch] } {
  error "b2 dont_touch changed after rejected remove_buffers"
}
if { [$b2 getPlacementStatus] ne "FIRM" } {
  error "b2 fixed placement changed after rejected remove_buffers"
}
if { ![$input_net isDoNotTouch] || ![$output_net isDoNotTouch] } {
  error "adjacent net dont_touch changed after rejected remove_buffers"
}
