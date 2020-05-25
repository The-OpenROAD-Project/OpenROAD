# welltap and spacing cells on core boundary
read_lef Nangate45/Nangate45.lef
read_lef extra.lef
read_def check8.def
set_placement_padding -global -right 1 -left 1
check_placement -verbose
