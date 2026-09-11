# A rejected user-selected buffer removal must not clear its dont_touch flag.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def remove_buffers3.def

# b3 prevents the merge when b2 is selected.  The override path may remove
# dont_touch after a successful eligibility check, but b2 must remain intact
# when the later merge check rejects it.
set_dont_touch b2
set_dont_touch b3
remove_buffers b2

set block [ord::get_db_block]
set b2 [$block findInst b2]
if {$b2 == "NULL" || ![$b2 isDoNotTouch]} {
  error "b2 dont_touch changed after rejected remove_buffers"
}
