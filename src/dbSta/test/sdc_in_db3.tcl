# An odb-only round trip preserves the record: with no liberty there is
# nothing to restore into, so read_db leaves the constraints in the block
# and write_db (without -sdc) carries them through for a later reader.
source "sdc_in_db_common.tcl"
read_lef Nangate45/Nangate45.lef
read_db sdc_in_db.odb
check "record carried" { ord::sdc_in_db_kind } native
check "nothing restored without liberty" { llength [all_clocks] } 0

set odb [make_result_file sdc_in_db3.odb]
write_db $odb
check "record still carried after write_db" { ord::sdc_in_db_kind } native

set after [make_result_file sdc_in_db3_after.sdc]
set ::env(SDC_IN_DB_ODB) $odb
set ::env(SDC_IN_DB_SDC) $after
set child [run_child sdc_in_db_restore.tcl]
check "linked reader sees the record" { child_kind $child } native
check "linked reader has the clocks" \
  { regexp -all {create_clock} [file_bytes $after] } 2
exit_summary
