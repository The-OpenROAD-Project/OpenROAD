# filler_placement size 2 3 4 8
read_lef Nangate45.lef
read_lef fill3.lef
read_def fillers4.def
filler_placement {FILLCELL_X2 FILLCELL_X3 FILLCELL_X4 FILLCELL_X8}
check_placement
