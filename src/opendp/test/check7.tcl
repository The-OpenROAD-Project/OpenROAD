# std cell abutting block set_placement_padding -right
read_lef Nangate45.lef
read_lef extra.lef
read_def check6.def
set_placement_padding -global -right 1
check_placement -verbose
