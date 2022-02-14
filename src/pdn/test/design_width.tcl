# test for error when adding straps that cannot fit in grid
source "helpers.tcl"

read_lef asap7_vias/asap7_tech_1x_arrayspacing.lef
read_lef asap7_vias/asap7sc7p5t_27_R_1x.lef
read_def asap7_vias/floorplan.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"

catch {add_pdn_stripe -layer M8 -width 2.0 -spacing 2.0 -pitch 20.0 -offset 10.0} err
puts $err
