# test for cut_pitch and fixed vias on connect statements
source "helpers.tcl"

read_lef asap7_vias/asap7_tech_1x.lef
read_lef asap7_vias/asap7sc7p5t_27_R_1x.lef
read_def asap7_vias/floorplan.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -layer M6 -width 1 -pitch 5 -offset 1
add_pdn_stripe -layer M7 -width 1 -pitch 5 -offset 1

add_pdn_connect -layers {M1 M2}
add_pdn_connect -layers {M2 M3}
add_pdn_connect -layers {M3 M6}
add_pdn_connect -layers {M6 M7}

pdngen -skip_trim

set def_file [make_result_file asap7_vias_cutclass.def]
write_def $def_file
diff_files asap7_vias_cutclass.defok $def_file
