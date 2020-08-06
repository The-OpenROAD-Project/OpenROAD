read_lef "input0.lef"
read_lef "input1.lef"
read_lef "input2.lef"
read_lef "input3.lef"
read_lef "input4.lef"

read_def "input.def"

fastroute -max_routing_layer 10 \
	  -unidirectional_routing \
	  -layers_adjustments {{2 0.5} {3 0.5}} \
	  -overflow_iterations 200 \
	  -verbose 2 \

exit
