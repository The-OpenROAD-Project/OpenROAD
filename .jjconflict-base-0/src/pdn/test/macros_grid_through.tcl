# test for define_pdn_grid -cells
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130_pll/pll.lef

read_def sky130_pll/floorplan.def

add_global_connection -net VDD -pin_pattern {^VPWR$} -power
add_global_connection -net VSS -pin_pattern {^VGND$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -grid "Core" -layer met4 -width 2.0 -pitch 40.0
add_pdn_stripe -grid "Core" -layer met5 -width 2.0 -pitch 40.0
add_pdn_connect -grid "Core" -layers {met4 met5}

define_pdn_grid -name "Macro" -macro -default
add_pdn_connect -grid "Macro" -layers {met4 met5}

pdngen

set def_file [make_result_file macros_grid_through.def]
write_def $def_file
diff_files macros_grid_through.defok $def_file
