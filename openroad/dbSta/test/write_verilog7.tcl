# write_verilog -remove_cells
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def write_verilog7.def

set verilog_file [make_result_file write_verilog7.v]
write_verilog -remove_cells FILL* $verilog_file
report_file $verilog_file
