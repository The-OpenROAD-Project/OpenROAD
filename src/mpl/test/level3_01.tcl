# 3 levels of registers between mem0 and mem1
source helpers.tcl
source level3.tcl
macro_placement -halo {0.5 0.5}

set def_file [make_result_file level3_01.def]
write_def $def_file
diff_file $def_file level3_01.defok
