# illegal placement; overlapping fixed cells
source helpers.tcl
read_lef Nangate45.lef
read_def simple06.def
detailed_placement
catch {check_placement} error
puts $error

set def_file [make_result_file simple06.def]
write_def $def_file
diff_file simple06.defok $def_file
