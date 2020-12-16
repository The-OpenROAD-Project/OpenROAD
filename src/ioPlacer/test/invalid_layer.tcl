# invalid routing layer
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def invalid_layer.def

catch {place_pins -hor_layers 2 -ver_layers 13} error
puts $error
