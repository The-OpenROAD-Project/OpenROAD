# The port out1 covers two colinear vertices of a straight route. The wire
# must bind the port once, at the middle of the contact, with no loop reported.
source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_def colinear_vertices_inside_bpin.def

set_extraction_rules_file 45_patterns.rules
extract_parasitics

set spef_file [make_result_file colinear_vertices_inside_bpin.spef]
write_spef $spef_file

diff_files colinear_vertices_inside_bpin.spefok $spef_file "^\\*(DATE|VERSION)"
