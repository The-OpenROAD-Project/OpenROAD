# write_verilog ignore vdd/vss def pins
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def write_verilog5.def

set verilog_file [make_result_file write_verilog5.v]
write_verilog -sort $verilog_file
report_file $verilog_file
