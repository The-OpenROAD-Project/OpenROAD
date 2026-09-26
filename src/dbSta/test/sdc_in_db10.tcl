# write_db without -sdc on a linked design drops the record: the caller
# chose not to store, and after edits the old record could be stale.
source "sdc_in_db_common.tcl"
load_libs
read_db sdc_in_db.odb
check "restored on read" { llength [all_clocks] } 2

set odb [make_result_file sdc_in_db10.odb]
write_db $odb
check "record dropped in this process" { ord::sdc_in_db_kind } none

set after [make_result_file sdc_in_db10_after.sdc]
set ::env(SDC_IN_DB_ODB) $odb
set ::env(SDC_IN_DB_SDC) $after
set child [run_child sdc_in_db_restore.tcl]
check "fresh reader finds none" { child_kind $child } none
exit_summary
