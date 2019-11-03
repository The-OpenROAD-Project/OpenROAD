# read def with hierarchical instance,net names
read_liberty liberty1.lib
read_lef liberty1.lef
read_def reg2.def
report_object_full_names [get_cells h1/r1]
report_object_full_names [get_cells h2/u1]
report_object_full_names [get_nets h1/r1q]
report_object_full_names [get_nets h2/u1z]
report_object_full_names [get_pins h1/r1/D]
report_object_full_names [get_pins h2/u1/A]
