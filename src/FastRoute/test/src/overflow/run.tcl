read_lef "../../input/nangate45/input0.lef"
read_lef "../../input/nangate45/input1.lef"
read_lef "../../input/nangate45/input2.lef"
read_lef "../../input/nangate45/input3.lef"
read_lef "../../input/nangate45/input4.lef"

read_def "../../input/nangate45/bp_fe_top.def"

fastroute -max_routing_layer 10 \
	  -unidirectional_routing \
	  -layers_adjustments {{2 0.5} {3 0.5}} \
	  -overflow_iterations 200 \
	  -verbose 2 \

exit
