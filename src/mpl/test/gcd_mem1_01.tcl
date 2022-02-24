# gcd 1 macro halo=1.0
source helpers.tcl
source gcd_mem1.tcl
macro_placement -halo {1.0 1.0}

set def_file [make_result_file gcd_mem1_01.def]
write_def $def_file
diff_file $def_file gcd_mem1_01.defok
