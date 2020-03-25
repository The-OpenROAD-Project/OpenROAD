# filler_placement list arg
read_lef Nangate45.lef
read_def simple01.def
detailed_placement
filler_placement {FILLCELL_X1 FILLCELL_X2}
check_placement
