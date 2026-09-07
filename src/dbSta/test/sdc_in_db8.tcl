# The native record is canonical and restoring it is a fixpoint: the same
# constraints give the same bytes from a second write_db in the same
# process and from a write_db in a fresh process that restored them from
# the .odb, and so do the .odb files themselves.
source "sdc_in_db_common.tcl"
load_design sdc_in_db.v
read_sdc sdc_in_db4.sdc

set a [make_result_file sdc_in_db8_a.odb]
set b [make_result_file sdc_in_db8_b.odb]
set c [make_result_file sdc_in_db8_c.odb]
write_db -sdc $a
set record_a [native_record]
write_db -sdc $b
set record_b [native_record]

set ::env(SDC_IN_DB_ODB) $a
set ::env(SDC_IN_DB_OUT) $c
set ::env(SDC_IN_DB_RECORD) [make_result_file sdc_in_db8_c.native]
run_child sdc_in_db_rewrite.tcl
set record_c [file_bytes $::env(SDC_IN_DB_RECORD)]

check "record, second write_db same process" { expr { $record_a eq $record_b } } 1
check "record, write_db after restore in a fresh process" \
  { expr { $record_a eq $record_c } } 1
check ".odb, second write_db same process" { same_file $a $b } 1
check ".odb, write_db after restore in a fresh process" { same_file $a $c } 1
exit_summary
