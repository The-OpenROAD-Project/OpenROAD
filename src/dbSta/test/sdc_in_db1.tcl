# write_db stores the timing constraints in the .odb, so an .odb is
# self-describing and does not have to be paired with a .sdc by name.
# With nothing beyond what the native form carries, that form is used
# (every pin an odb object id), and only that form.
source "sdc_in_db_common.tcl"
load_design sdc_in_db.v
read_sdc sdc_in_db1.sdc

write_db [make_result_file sdc_in_db1.odb]
check "stored form" { ord::sdc_in_db_kind } native
check "native property present" { has_property "sta.sdc.native" } 1
check "text property absent" { has_property "sta.sdc" } 0
check "record header" { lindex [split [native_record] \n] 0 } "sdc-in-odb 1"
check "one clock record per clock" \
  { regexp -all -line {^C } [native_record] } [llength [all_clocks]]
exit_summary
