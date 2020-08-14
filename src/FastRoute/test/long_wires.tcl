source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set guide_file [make_result_file long_wires.guide]

fastroute -output_file $guide_file \
          -max_routing_layer 10 \
          -unidirectional_routing \
          -max_routing_length 10.0 \
          -max_length_per_layer { {2 20.0} {3 15.0} {5 5.0}} \

exit
