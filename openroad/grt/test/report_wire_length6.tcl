read_lef report_wire_length6.lef1
read_lef report_wire_length6.lef2

read_def report_wire_length6.def

set_global_routing_layer_adjustment M2-M7 0.3
set_routing_layers -signal M2-M7

read_guides report_wire_length6.guide

report_wire_length -net * -global_route
