# Without -sdc, read_db ignores the constraints stored in the .odb, so
# scripts that read their own .sdc see exactly the behavior they saw before.
source "sdc_in_db_common.tcl"
load_libs
read_db sdc_in_db.odb

check "constraints are stored" { ord::sdc_in_db_kind } native
check "but not restored" { llength [all_clocks] } 0
exit_summary
