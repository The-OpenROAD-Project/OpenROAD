# A buffer removal with instance names that match nothing must fail instead of
# falling back to removing every buffer in the design.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def remove_buffers1.def

set block [ord::get_db_block]
set insts_before [llength [$block getInsts]]

set rc [catch { remove_buffers no_such_inst } msg]
if { $rc == 0 } {
  error "remove_buffers with an unmatched instance name should fail"
}
if { [llength [$block getInsts]] != $insts_before } {
  error "remove_buffers changed the design after failing"
}

# Omitting the instance list still removes every buffer.
remove_buffers
if { [llength [$block getInsts]] != 0 } {
  error "remove_buffers without arguments should remove all buffers"
}

puts "pass"
