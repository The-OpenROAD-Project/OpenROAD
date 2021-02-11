# 3 levels of registers between mem0 and mem1
source helpers.tcl
source helpers.tcl

read_liberty Nangate45/Nangate45_typ.lib
read_liberty Nangate45/fakeram45_64x7.lib

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x7.lef

read_verilog level1.v
link_design gcd_mem3
read_sdc gcd.sdc

if {1} {
initialize_floorplan -die_area {0 0 58.14 56.0} \
  -core_area {0 0 58.14 56.0} \
  -site FreePDK45_38x28_10R_NP_162NW_34O \
  -tracks Nangate45/nangate45.tracks
} else {
initialize_floorplan -die_area {0 0 70 70} \
  -core_area {0 0 60 60} \
  -site FreePDK45_38x28_10R_NP_162NW_34O \
  -tracks Nangate45/nangate45.tracks
}

place_pins -random -hor_layers 3 -ver_layers 2

global_placement -disable_routability_driven
macro_placement -global_config halo_0.5.cfg

set def_file [make_result_file level1.def]
write_def $def_file
diff_file $def_file level1.defok
