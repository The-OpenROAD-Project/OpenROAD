# test for min width violation
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"

# tclint-disable-next-line line-length
catch {add_pdn_ring -grid "Core" -layers {metal5 metal6} -widths 0.1 -spacings 2.0 -core_offsets 2.0} err
puts $err
