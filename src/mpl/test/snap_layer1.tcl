# -snap_layer 3
source helpers.tcl
set mem0_pins_west 1
source east_west.tcl

macro_placement -halo {1.0 1.0} -snap_layer 3

set def_file [make_result_file snap_layer1.def]
write_def $def_file
diff_file $def_file snap_layer1.defok
