# mem0 with east connctions, mem1 with west connections
source helpers.tcl
set mem0_pins_west 0
source east_west.tcl

# Note that the default -style MAXIMIZES wire length and incorrectly
# places the memories away from the pins they are connected to.
macro_placement -style corner_min_wl -halo {0.5 0.5}

set def_file [make_result_file east_west2.def]
write_def $def_file
diff_file $def_file east_west2.defok
