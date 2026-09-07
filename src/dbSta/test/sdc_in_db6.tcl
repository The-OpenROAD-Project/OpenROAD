# A design written without constraints carries none: read_db -sdc says
# so (returns 0) instead of pretending, so a flow can fall back to its
# .sdc file.
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog reg1.v
link_design top

set odb [make_result_file sdc_in_db6.odb]
write_db $odb
puts "stored form: [ord::sdc_in_db_kind]"

set after [make_result_file sdc_in_db6_after.sdc]
set ::env(SDC_IN_DB_ODB) $odb
set ::env(SDC_IN_DB_SDC) $after
puts [exec $argv0 -no_init -no_splash -exit sdc_in_db_restore.tcl]
