# set_placement_padding + cell->width > row width
source helpers.tcl
read_lef Nangate45.lef
read_def simple01.def
set_placement_padding -global -left 30
detailed_placement
check_placement -verbose
