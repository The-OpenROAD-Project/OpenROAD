# No mode is silently lost. The record accounts for every mode sta holds,
# named, so a design with more modes than the writer happened to be in
# cannot be stored as if it had only one.
source "sdc_in_db_common.tcl"
load_design sdc_in_db.v

set_mode func
create_clock -name clk -period 10 [get_ports clk1]
set_mode test
create_clock -name clk -period 20 [get_ports clk1]

write_db -sdc [make_result_file sdc_in_db13.odb]
check "stored form" { ord::sdc_in_db_kind } native
check "one record section per mode" \
  { regexp -all -line {^M } [native_record] } [llength [get_modes *]]
check "every mode named in the record" { record_modes } {func test}
exit_summary
