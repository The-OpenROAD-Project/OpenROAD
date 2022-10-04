# get_cells/pins with brackets
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_def sdc_names2.def

report_object_full_names [get_cells foo[0].bar[2].baz]
report_object_full_names [get_pins foo[0].bar[2].baz/A]
report_edges -to foo[0].bar[2].baz/Z
