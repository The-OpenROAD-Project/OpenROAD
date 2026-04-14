# ram with obstruction
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x7.lef
read_def obstruction2.def
catch { detailed_placement } error
puts $error
