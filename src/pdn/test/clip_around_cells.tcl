# test for clip_around_cells, replicating a guardring with metal1 elements on nangate45

source "helpers.tcl"

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef nangate_guardring/nangate45_fake_guardring.lef
read_def nangate_guardring/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern {^VSS|RING$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1 -clip_around_cells

add_pdn_stripe -layer metal4 -width 1.0 -pitch 10.0 -offset 2.5 -extend_to_core_ring

add_pdn_connect -layers {metal1 metal4}

pdngen

set def_file [make_result_file clip_around_cells.def]
write_def $def_file
diff_files clip_around_cells.defok $def_file

