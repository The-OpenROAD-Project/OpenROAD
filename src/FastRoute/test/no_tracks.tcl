# design with no routing tracks
read_lef Nangate45/Nangate45.lef
read_lib Nangate45/Nangate45_typ.lib
read_verilog ./no_tracks.v
link_design gcd

initialize_floorplan -site FreePDK45_38x28_10R_NP_162NW_34O \
					 -utilization 30

catch {global_route -layers 2-10} error
puts $error

