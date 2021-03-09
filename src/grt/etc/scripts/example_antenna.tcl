read_lef "input.lef"
read_def "input.def"

fastroute -output_file "route.guide" \
	  -max_routing_layer 10 \
	  -unidirectional_routing \
	  -max_routing_length 10.0 \
	  -max_length_per_layer {{1 12.0} {2 30.0} {3 50.0} {7 100.0}} \

exit

# To fix long segments, the option -max_routing_length must be used. It sets a
# global max length, that will be used to all layers that does not have specific
# max routing length
#
# The option -max_length_per_layer receives a list of pairs. Each pair contains
# the layer index (beginning in 1) and the max routing length allowed for this
# layer
