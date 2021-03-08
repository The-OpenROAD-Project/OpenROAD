# write_verilog bus ports reversed bit order
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog reg5.v
link_design reg5

set verilog_file [make_result_file write_verilog8.v]
write_verilog $verilog_file
report_file $verilog_file
