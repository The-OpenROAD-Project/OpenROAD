# gcd 5 macros halo=0.5
source helpers.tcl
source gcd_mem5.tcl
macro_placement -halo {0.5 0.5}

set def_file [make_result_file gcd_mem5_01.def]
write_def $def_file
diff_file $def_file gcd_mem5_01.defok
