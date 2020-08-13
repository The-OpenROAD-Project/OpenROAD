read_lef "../../input/nangate45/input0.lef"
read_lib "../../input/nangate45/input.lib"
read_verilog "../../input/nangate45/input.v"
link_design gcd
read_sdc "../../input/nangate45/input.sdc"

initialize_floorplan -site FreePDK45_38x28_10R_NP_162NW_34O \
		     -utilization 30 

fastroute -max_routing_layer 10

exit
