# Round trip through the native form, across processes: write_sdc from
# the design that read the .sdc must equal write_sdc from a fresh process
# that got its constraints from the .odb alone.
source "sdc_in_db_common.tcl"
load_design sdc_in_db.v
read_sdc sdc_in_db4.sdc

set odb [make_result_file sdc_in_db4.odb]
write_db -sdc $odb
check "stored form" { ord::sdc_in_db_kind } native
set before [make_result_file sdc_in_db4_before.sdc]
write_sdc -no_timestamp $before

set after [make_result_file sdc_in_db4_after.sdc]
set ::env(SDC_IN_DB_ODB) $odb
set ::env(SDC_IN_DB_SDC) $after
set child [run_child sdc_in_db_restore.tcl]
check "form seen by the restoring process" { child_kind $child } native
check "write_sdc before and after restore" { same_file $before $after } 1
exit_summary
