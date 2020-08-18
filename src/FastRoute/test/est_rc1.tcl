source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set guide_file [make_result_file gcd_route.guide]

fastroute -output_file $guide_file \
      -max_routing_layer 10 \
      -unidirectional_routing \
      -estimateRC

set_cmd_units -capacitance ff
report_net -connections -verbose clk
