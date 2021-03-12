# filler_placement size 2 3 4 8 with gap of 1 (error)
read_lef Nangate45/Nangate45.lef
read_lef fill3.lef
read_def fillers6.def
catch {filler_placement {FILLCELL_X2 FILLCELL_X3 FILLCELL_X4 FILLCELL_X8}} msg

