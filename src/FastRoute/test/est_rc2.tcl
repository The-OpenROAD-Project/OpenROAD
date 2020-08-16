cd ~/openroad/test
source "helpers.tcl"
source Nangate45/Nangate45.vars
read_lef $tech_lef
read_lef $std_cell_lef
read_liberty $liberty_file
read_def ~/openroad/src/FastRoute/test/est_rc2.def

set route_guide [make_result_file est_rc2.route_guide]

fastroute -output_file $route_guide\
  -max_routing_layer $max_routing_layer \
  -unidirectional_routing true \
  -layers_adjustments $layers_adjustments \
  -layers_pitches $layers_pitches \
  -overflow_iterations 100 \
  -verbose 2

estimate_parasitics -global_routing
report_net -connections -verbose -digits 3 clk
report_net -connections -verbose -digits 3 u2/ZN
