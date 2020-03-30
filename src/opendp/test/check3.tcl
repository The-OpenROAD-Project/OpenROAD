# abutting cells / overlap padded
read_lef Nangate45.lef
read_def check3.def
set_placement_padding -global -left 1 -right 1
check_placement -verbose
