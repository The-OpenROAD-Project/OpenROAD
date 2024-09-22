# test for error checking against the manufacturing grid
source "helpers.tcl"

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
catch {add_pdn_stripe -layer metal1 -width 0.075 -pitch 4} err
puts $err

catch {add_pdn_stripe -layer metal1 -width 0.07 -pitch 4.004} err
puts $err

catch {add_pdn_stripe -layer metal1 -width 0.07 -spacing 0.504 -pitch 4} err
puts $err
