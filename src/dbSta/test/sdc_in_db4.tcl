# Round trip through the native form, across processes: write_sdc from
# the design that read the .sdc must equal write_sdc from a fresh process
# that got its constraints from the .odb alone.
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog reg1.v
link_design top
read_sdc sdc_in_db4.sdc

set odb [make_result_file sdc_in_db4.odb]
write_db $odb
puts "stored form: [ord::sdc_in_db_kind]"
set before [make_result_file sdc_in_db4_before.sdc]
write_sdc -no_timestamp $before

set after [make_result_file sdc_in_db4_after.sdc]
set ::env(SDC_IN_DB_ODB) $odb
set ::env(SDC_IN_DB_SDC) $after
puts [exec $argv0 -no_init -no_splash -exit sdc_in_db_restore.tcl]

report_file $after
diff_files $before $after
