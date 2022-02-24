# mem0 with west connctions, mem1 with east connections
source helpers.tcl
set mem0_pins_west 1
source east_west.tcl
read_sdc gcd.sdc

# Note that the default -style MAXIMIZES wire length and incorrectly
# places the memories away from the pins they are connected to.
macro_placement -style corner_min_wl -halo {0.5 0.5}

set def_file [make_result_file east_west1.def]
write_def $def_file
diff_file $def_file east_west1.defok
