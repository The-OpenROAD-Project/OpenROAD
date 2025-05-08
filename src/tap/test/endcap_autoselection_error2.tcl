source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef Nangate45_data/endcaps.lef
read_lef Nangate45_data/endcaps_symmetric.lef
read_lef Nangate45/fakeram45_64x7.lef
read_def Nangate45_data/macros.def

set def_file [make_result_file boundary_macros_tapcell.def]

catch {place_endcaps} error

puts $error