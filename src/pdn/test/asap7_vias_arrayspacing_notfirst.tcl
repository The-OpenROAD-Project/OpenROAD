# test for picking the best via array possible
source "helpers.tcl"

read_lef asap7_vias/asap7_tech_1x_arrayspacing_tightspacing.lef
read_lef asap7_vias/asap7sc7p5t_27_R_1x.lef
read_def asap7_vias/floorplan.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"

add_pdn_stripe -layer M8 -width 2.0 -spacing 2.0 -pitch 20.0 -offset 1.0
add_pdn_stripe -layer M9 -width 2.0 -spacing 2.0 -pitch 20.0 -offset 1.0

add_pdn_connect -layers {M8 M9}

pdngen -skip_trim

set def_file [make_result_file asap7_vias_arrayspacing_notfirst.def]
write_def $def_file
diff_files asap7_vias_arrayspacing_notfirst.defok $def_file
