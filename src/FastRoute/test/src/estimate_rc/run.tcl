read_liberty "../../../../../test/Nangate45/Nangate45_typ.lib"
read_lef "../../input/nangate45/input0.lef"
read_def "../../input/nangate45/gcd.def"

fastroute -output_file "route.guide" \
	  -max_routing_layer 10 \
	  -unidirectional_routing
estimate_parasitics -global_routing

report_net -connections -verbose clk
exit
