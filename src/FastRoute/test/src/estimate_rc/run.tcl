read_lef "../../input/nangate45/input0.lef"
read_def "../../input/nangate45/gcd.def"

fastroute -output_file "route.guide" \
	  -max_routing_layer 10 \
	  -unidirectional_routing \
	  -estimateRC

set_cmd_units -capacitance ff
report_net -connections -verbose clk
exit
