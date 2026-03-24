# init_floorplan
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top

catch {
  initialize_floorplan -utilization 99 \
    -aspect_ratio 0.2 \
    -core_space 1 \
    -site FreePDK45_38x28_10R_NP_162NW_34O
} err
puts $err
