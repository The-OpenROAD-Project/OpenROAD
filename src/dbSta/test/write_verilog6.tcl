# write_verilog -include_pwr_gnd_pins
source "helpers.tcl"
read_lef liberty1.lef
read_def reg1.def

set verilog_file [make_result_file write_verilog6.v]
write_verilog  -include_pwr_gnd $verilog_file
report_file $verilog_file
