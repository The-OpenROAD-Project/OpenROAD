# read_verilog 16b bus with lef/liberty
source "helpers.tcl"
read_lef bus1.lef
read_liberty bus1.lib
read_verilog bus1.v
link_design top
report_instance bus1
