read_lef "../../input/nangate45/input0.lef"
read_def "../../input/nangate45/gcd.def"

fastroute -output_file "route.guide" \
	  -max_routing_layer 10 \
	  -unidirectional_routing \
	  -max_routing_length 10.0 \
	  -max_length_per_layer { {2 20.0} {3 15.0} {5 5.0}} \

exit

