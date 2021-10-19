source "helpers.tcl"

read_lef asap7_vias/asap7_tech_1x_201209.lef
read_lef asap7_vias/asap7sc7p5t_27_R_1x_201211.lef
read_def asap7_vias/2_5_floorplan_tapcell.def

set_voltage_domain -name CORE -power VDD -ground VSS

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

define_pdn_grid -name grid -starts_with POWER 
add_pdn_stripe -layer M1 -width 0.072 -offset 0 -followpins
add_pdn_stripe -layer M2 -width 0.072 -offset 0 -followpins
add_pdn_stripe -layer M3 -width 0.936 -spacing 1.512 -pitch 31.68 -offset 1.872
add_pdn_stripe -layer M6 -width 1.152 -spacing 2.688 -pitch 56.32 -offset 6.408
add_pdn_connect -layers {M1 M2} -cut_pitch 0.288
add_pdn_connect -layers {M2 M3} -fixed_vias VIA23
add_pdn_connect -layers {M3 M6} -fixed_vias {VIA45 VIA56}

pdngen -verbose

set def_file results/asap7_vias.def
write_def $def_file 

diff_files $def_file asap7_vias.defok
