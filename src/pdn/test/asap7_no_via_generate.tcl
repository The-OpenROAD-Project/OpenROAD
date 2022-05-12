# test for no via generate statements in LEF using just fixed vias
source "helpers.tcl"

read_lef asap7_vias/asap7_tech_1x_noviarules.lef
read_lef asap7_vias/asap7sc7p5t_27_R_1x.lef
read_def asap7_vias/floorplan.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer M1 -width 0.072
add_pdn_stripe -followpins -layer M2 -width 0.090

add_pdn_stripe -layer M3 -width 0.936 -spacing 1.512 -pitch 8.00 -offset 1.000
add_pdn_stripe -layer M6 -width 1.152 -spacing 2.500 -pitch 8.00 -offset 1.000

add_pdn_connect -layers {M1 M2} -cut_pitch 0.288 -fixed_vias "VIA12"
add_pdn_connect -layers {M2 M3} -fixed_vias "VIA23"
add_pdn_connect -layers {M3 M6} -fixed_vias "VIA34 VIA45 VIA56"

pdngen

set def_file [make_result_file asap7_no_via_generate.def]
write_def $def_file
diff_files asap7_no_via_generate.defok $def_file
