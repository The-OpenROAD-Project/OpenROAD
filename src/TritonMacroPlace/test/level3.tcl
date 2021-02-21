# 3 levels of registers between mem0 and mem1
source helpers.tcl

read_liberty Nangate45/Nangate45_typ.lib
read_liberty Nangate45/fakeram45_64x7.lib

read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x7.lef

# place_pins result is not stable across ports. sigh.
if {0} {
read_verilog level3.v
link_design gcd_mem3
read_sdc gcd.sdc

initialize_floorplan -die_area {0 0 58.14 56.0} \
  -core_area {0 0 58.14 56.0} \
  -site FreePDK45_38x28_10R_NP_162NW_34O \
  -tracks Nangate45/Nangate45.tracks

place_pins -random -hor_layers 3 -ver_layers 2
write_def level3.def
} else {
read_def level3.def
read_sdc gcd.sdc
}

global_placement
macro_placement -halo {0.5 0.5}

set def_file [make_result_file level3.def]
write_def $def_file
diff_file $def_file level3.defok
