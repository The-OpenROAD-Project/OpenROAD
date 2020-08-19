# invalid routing layer
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def invalid_layer.def

io_placer -hor_layer 2 -ver_layer 13
