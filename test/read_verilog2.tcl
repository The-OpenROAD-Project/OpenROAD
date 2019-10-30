# hieararchical verilog
source "helpers.tcl"
read_lef -tech -library liberty1.lef
read_verilog read_verilog2.v
link_design top

set db_file [make_result_file read_verilog2.db]
write_db $db_file

set def_file [make_result_file read_verilog2.def]
write_def $def_file
report_file $def_file
