# test for rejecting connect rules with same layer
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"

catch {add_pdn_connect -layers {metal5 metal5}} err
puts $err

catch {add_pdn_connect -layers {via1 metal5}} err
puts $err

catch {add_pdn_connect -layers {metal1 via2}} err
puts $err
