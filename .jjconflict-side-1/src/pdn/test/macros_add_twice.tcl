# test for adding instnace to two different grids
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_macros/fakeram45_64x32.lef

read_def nangate_macros/floorplan.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VDD -pin_pattern {^VDDPE$}
add_global_connection -net VDD -pin_pattern {^VDDCE$}
add_global_connection -net VSS -pin_pattern {^VSS$} -ground
add_global_connection -net VSS -pin_pattern {^VSSE$}

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
define_pdn_grid -macro -name "sram1" -instances "dcache.data.data_arrays_0.data_arrays_0_ext.mem"
define_pdn_grid -macro -name "sram2" -instances "dcache.data.data_arrays_0.data_arrays_0_ext.mem"
