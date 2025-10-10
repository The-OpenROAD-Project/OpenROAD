# Test write_verilog -remove_cells in a hierarchical flow
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog write_verilog9.v
link_design -hier top

set verilog_file [make_result_file write_verilog9_hier.v]
write_verilog -remove_cells BUF* $verilog_file
report_file $verilog_file
