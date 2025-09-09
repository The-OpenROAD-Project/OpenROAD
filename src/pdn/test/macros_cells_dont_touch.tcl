# test for global connect dont_touch
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_macros/fakeram45_64x32.lef

read_def nangate_macros/floorplan.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VDD -pin_pattern {^VDDPE$}
add_global_connection -net VDD -pin_pattern {^VDDCE$}
add_global_connection -net VSS -pin_pattern {^VSS$} -ground
add_global_connection -net VSS -pin_pattern {^VSSE$}

set_dont_touch dcache.data.data_arrays_0.data_arrays_0_ext.mem
set_dont_touch VSS

# except all but the "dcache.data.data_arrays_0.data_arrays_0_ext.mem" to be connected to VDD2
add_global_connection -net VDD2 -pin_pattern {^VDD$} -power
add_global_connection -net VDD2 -pin_pattern {^VDDPE$}
add_global_connection -net VDD2 -pin_pattern {^VDDCE$}
# expect no connections to VSS2
add_global_connection -net VSS2 -pin_pattern {^VSS$} -ground
add_global_connection -net VSS2 -pin_pattern {^VSSE$}

set def_file [make_result_file macros_cells_dont_touch.def]
write_def $def_file
diff_files macros_cells_dont_touch.defok $def_file
