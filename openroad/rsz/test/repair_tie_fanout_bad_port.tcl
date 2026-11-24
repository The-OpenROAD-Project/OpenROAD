source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_tie1.def

catch { repair_tie_fanout BOGUS_PIN } error
puts $error
