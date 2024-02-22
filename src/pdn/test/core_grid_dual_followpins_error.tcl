# test for error checking connect statements across multiple followpins
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -followpins -layer metal2

add_pdn_connect -layers {metal1 metal3}

## error due to incorrect connect statement
catch {pdngen} err
puts $err

## error due to missing connect statement
add_pdn_stripe -followpins -layer metal3
catch {pdngen} err
puts $err

## error due to overlapping connect statements
add_pdn_connect -layers {metal1 metal2}
catch {pdngen} err
puts $err
