# gcd 3 macros halo=1.0
source helpers.tcl
source gcd_mem3.tcl
macro_placement -halo {1.0 1.0}

set def_file [make_result_file gcd_mem3_01.def]
write_def $def_file
diff_file $def_file gcd_mem3_01.defok
