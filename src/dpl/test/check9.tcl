# check_placement block off grid (no error)
read_lef Nangate45/Nangate45.lef
read_lef extra.lef
read_def check9.def
check_placement -verbose
