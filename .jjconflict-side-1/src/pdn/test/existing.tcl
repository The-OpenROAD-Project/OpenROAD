# test for adding vias to an existing grid
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_macros/fakeram45_64x32.lef

read_def nangate_existing/floorplan.def

define_pdn_grid -existing

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}
add_pdn_connect -layers {metal7 metal8}

add_pdn_connect -layers {metal4 metal5}
add_pdn_connect -layers {metal5 metal6}
add_pdn_connect -layers {metal6 metal7}

pdngen

set def_file [make_result_file existing.def]
write_def $def_file
diff_files existing.defok $def_file
