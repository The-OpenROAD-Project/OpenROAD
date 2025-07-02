# test for split vias in followpins
source "helpers.tcl"

read_lef asap7_vias/asap7_tech_1x.lef
read_lef asap7_vias/asap7sc7p5t_27_R_1x.lef
read_def asap7_vias/floorplan.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer M1 -width 0.072
add_pdn_stripe -followpins -layer M3 -width 0.090

add_pdn_connect -layers {M1 M3} -split_cuts {M2 1.0}

pdngen

set def_file [make_result_file asap7_M1_M3_followpins.def]
write_def $def_file
diff_files asap7_M1_M3_followpins.defok $def_file
