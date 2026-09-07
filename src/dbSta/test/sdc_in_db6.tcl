# A design written without constraints carries none: read_db -sdc has
# nothing to restore and says so through sdc_in_db_kind, so a flow can
# fall back to its .sdc file.
source "sdc_in_db_common.tcl"
load_design sdc_in_db.v

set odb [make_result_file sdc_in_db6.odb]
write_db $odb
check "stored form" { ord::sdc_in_db_kind } none
check "no native property" { has_property "sta.sdc.native" } 0
check "no text property" { has_property "sta.sdc" } 0

set after [make_result_file sdc_in_db6_after.sdc]
set ::env(SDC_IN_DB_ODB) $odb
set ::env(SDC_IN_DB_SDC) $after
set child [run_child sdc_in_db_restore.tcl]
check "form seen by the restoring process" { child_kind $child } none
check "restoring process has no clocks" { file_matches $after {create_clock} } 0
exit_summary
