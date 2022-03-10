# gcd 1 macro -style corner_min_wl
source helpers.tcl
source gcd_mem1.tcl
macro_placement -style corner_min_wl -channel {2.0 2.0}

set def_file [make_result_file gcd_mem1_03.def]
write_def $def_file
diff_file $def_file gcd_mem1_03.defok
