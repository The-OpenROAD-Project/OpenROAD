# test for violation of width table on M2
source "helpers.tcl"

read_lef asap7_vias/asap7_tech_1x_noviarules.lef
read_lef asap7_vias/asap7sc7p5t_27_R_1x.lef
read_def asap7_vias/floorplan.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer M1 -width 0.072
catch {add_pdn_stripe -followpins -layer M2 -width 0.072} err
puts $err
