# filler_placement with verbose
source "helpers.tcl"
read_lef fillers9.lef
read_def fillers9.def

filler_placement FILL* -verbose
