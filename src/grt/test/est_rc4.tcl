# estimate parasitics based on gr results
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_db "est_rc4.odb"

set_routing_layers -signal metal2-metal10
estimate_parasitics -global_routing

report_net -connections -verbose -digits 3 clk
