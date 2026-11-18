# write_sdc
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog hier1.v
link_design top
read_sdc hier1.sdc
set sdc_file [make_result_file write_sdc1.sdc]
write_sdc -no_timestamp $sdc_file
report_file $sdc_file
