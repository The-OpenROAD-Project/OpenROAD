# Two output ports in a child module whose Verilog escaped-identifier
# names differ only in a middle path segment must round-trip through
# write_verilog as two distinct ports, not collapse to a duplicate.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog hier_escape_port.v
link_design -hier top

set verilog_file [make_result_file hier_escape_port.v]
write_verilog $verilog_file
report_file $verilog_file
