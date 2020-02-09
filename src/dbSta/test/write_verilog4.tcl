# write_verilog DEF equiv to verilog quoted bus port bus input [1:0] \in[0] ;
source "helpers.tcl"
read_lef liberty1.lef
read_def write_verilog4.def

set verilog_file [make_result_file write_verilog4.v]
write_verilog  $verilog_file
report_file $verilog_file
