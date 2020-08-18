read_lef Nangate45/Nangate45.lef
read_lib Nangate45/Nangate45_typ.lib
read_verilog ./no_tracks.v
link_design gcd

initialize_floorplan -site FreePDK45_38x28_10R_NP_162NW_34O \
-utilization 30

fastroute -max_routing_layer 10
