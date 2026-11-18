# report_overdriven_nets with 1 net
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog report_overdriven_nets2.v
link_design top
report_overdriven_nets
report_overdriven_nets -verbose
