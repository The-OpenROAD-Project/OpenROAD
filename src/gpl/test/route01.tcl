source helpers.tcl
read_lef nangate45.lef
read_def medium01.def

grt::add_layer_adjustment 2 0.4
grt::add_layer_adjustment 3 0.4
grt::add_layer_adjustment 4 0.2
grt::add_layer_adjustment 5 0.2
grt::add_layer_adjustment 6 0.2
grt::add_layer_adjustment 7 0.2
grt::add_layer_adjustment 8 0.2
grt::add_layer_adjustment 9 0.2
grt::add_layer_adjustment 10 0.2

grt::set_unidirectional_routing true

# fastroute -write_route -write_est -max_routing_layer 3
global_placement -routability_driven \
  -routability_rc_coefficients {1 1 0 0} \
  -verbose 1 \
  -skip_initial_place -density 0.5 \
  -pad_left 1 -pad_right 1 
