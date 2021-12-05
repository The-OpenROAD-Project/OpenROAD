# gcd 3 macros halo=0.5 channel=2.0
source helpers.tcl
source gcd_mem3.tcl
macro_placement -halo {0.5 0.5} -channel {2.0 2.0}

set def_file [make_result_file gcd_mem3_02.def]
write_def $def_file
diff_file $def_file gcd_mem3_02.defok
