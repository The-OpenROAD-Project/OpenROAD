# Round trip through the text fallback, across processes: a derate is
# outside the native form, so the constraints ride as write_sdc text and
# are replayed. The result must still equal what was written.
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog reg1.v
link_design top
read_sdc sdc_in_db5.sdc

set odb [make_result_file sdc_in_db5.odb]
write_db $odb
puts "stored form: [ord::sdc_in_db_kind]"
set before [make_result_file sdc_in_db5_before.sdc]
write_sdc -no_timestamp $before

set after [make_result_file sdc_in_db5_after.sdc]
set ::env(SDC_IN_DB_ODB) $odb
set ::env(SDC_IN_DB_SDC) $after
puts [exec $argv0 -no_init -no_splash -exit sdc_in_db_restore.tcl]

report_file $after
diff_files $before $after
