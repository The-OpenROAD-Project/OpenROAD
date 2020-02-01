# write_verilog from lef macro bus pins
source "helpers.tcl"
read_lef -tech liberty1.lef
read_lef bus1.lef
read_def bus1.def
read_liberty bus1.lib

set verilog_file [make_result_file write_verilog3.v]
write_verilog $verilog_file
report_file $verilog_file
