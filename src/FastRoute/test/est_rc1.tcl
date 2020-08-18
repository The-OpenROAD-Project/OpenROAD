source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_liberty Nangate45/Nangate45_typ.lib
read_def "gcd.def"

set guide_file [make_result_file gcd_route.guide]

fastroute -output_file $guide_file \
      -max_routing_layer 10 \
      -unidirectional_routing
estimate_parasitics -global_routing

report_net -connections -verbose -digits 3 clk
