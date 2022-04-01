# gcd 3 macros -style corner_min_wl
source helpers.tcl
source gcd_mem3.tcl
macro_placement -style corner_min_wl -channel {2.0 2.0}

set def_file [make_result_file gcd_mem3_03.def]
write_def $def_file
diff_file $def_file gcd_mem3_03.defok
