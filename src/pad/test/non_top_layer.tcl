# Test for placing pads
source "helpers.tcl"

# Init chip
read_lef Nangate45/Nangate45.lef

# IO pad with no top level pin shape
read_lef non_top_layer.lef

read_def non_top_layer.def

place_io_terminals -allow_non_top_layer */PAD
catch {place_io_terminals */PAD}
