# hierarchical names
source "helpers.tcl"
read_lef liberty1.lef
read_def hier1.def
init_sta_db
report_object_full_names [get_cells b1/r1]
report_object_full_names [get_cells b1/*]
