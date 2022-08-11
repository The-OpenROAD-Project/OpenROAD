read_lef read_guides1.lef1
read_lef read_guides1.lef2

read_def read_guides1.def

set_global_routing_layer_adjustment M2-M7 0.3
set_routing_layers -signal M2-M7

read_guides read_guides1.guide

report_wire_length -net * -global_route