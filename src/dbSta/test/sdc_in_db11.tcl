# A stale record is rejected: the native form binds itself to the names
# of the objects it refers to, so constraints written for one design are
# not silently applied to another. The stale case is built odb-only (no
# liberty, so no restore), by renaming a referenced instance and writing
# the block out with the record untouched.
source "sdc_in_db_common.tcl"
load_design sdc_in_db.v
read_sdc sdc_in_db4.sdc

set odb [make_result_file sdc_in_db11.odb]
write_db -sdc $odb
check "stored form" { ord::sdc_in_db_kind } native

set stale [make_result_file sdc_in_db11_stale.odb]
set ::env(SDC_IN_DB_ODB) $odb
set ::env(SDC_IN_DB_OUT) $stale
set odb_only [run_child sdc_in_db_rename.tcl]
check "odb-only process carried the record" \
  { regexp -all {stored form(?: after write_db)?: native} $odb_only } 2

set ::env(SDC_IN_DB_ODB) $stale
set ::env(SDC_IN_DB_SDC) [make_result_file sdc_in_db11_after.sdc]
set child [run_child sdc_in_db_restore.tcl]
check "linked reader rejects the stale record" \
  { regexp {read_db failed: STA-3012} $child } 1
exit_summary
