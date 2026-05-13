# This is a version of aes where some of the gates have been unplaced to
# simulate rmp.

source helpers.tcl
set test_name incremental02
read_lef ./nangate45.lef
read_def ./$test_name.def

set_thread_count 4
global_placement -incremental -density 0.3 -pad_left 2 -pad_right 2
set def_file [make_result_file $test_name.def]
write_def $def_file

set unplaced 0
set block [ord::get_db_block]
foreach inst [$block getInsts] {
  if { [$inst getPlacementStatus] == "NONE" } {
    incr unplaced
  }
}

if { $unplaced != 0 } {
  error "Expected all instances to be placed, found $unplaced unplaced instances"
}

puts pass
