# Round trip through the text fallback for constructs the Sdc inspection
# flags (a generated clock, a port load): the constraints ride as
# write_sdc text, are replayed, and still equal what was written.
source "sdc_in_db_common.tcl"
load_design sdc_in_db.v
read_sdc sdc_in_db9.sdc

set odb [make_result_file sdc_in_db9.odb]
write_db -sdc $odb
check "stored form" { ord::sdc_in_db_kind } text
set before [make_result_file sdc_in_db9_before.sdc]
write_sdc -no_timestamp $before

set after [make_result_file sdc_in_db9_after.sdc]
set ::env(SDC_IN_DB_ODB) $odb
set ::env(SDC_IN_DB_SDC) $after
set child [run_child sdc_in_db_restore.tcl]
check "form seen by the restoring process" { child_kind $child } text
check "write_sdc before and after restore" { same_file $before $after } 1
exit_summary
