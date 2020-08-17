source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def est_rc2.def

set route_guide [make_result_file est_rc2.route_guide]

fastroute -output_file $route_guide\
  -max_routing_layer 10 \
  -unidirectional_routing true
estimate_parasitics -global_routing

report_net -connections -verbose -digits 3 clk
report_net -connections -verbose -digits 3 u2/ZN
exit
