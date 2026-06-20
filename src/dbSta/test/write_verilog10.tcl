# Repro for issue #10414: multiple power/ground pins tied to one net
# whose name differs from the port names.  write_verilog -include_pwr_gnd
# must emit assign statements linking each port to the net, otherwise the
# power connection is dropped and LVS breaks.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def write_verilog10.def

set verilog_file [make_result_file write_verilog10.v]
write_verilog -include_pwr_gnd $verilog_file
report_file $verilog_file
