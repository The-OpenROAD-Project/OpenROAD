# 3 levels of registers between mem0 and mem1, -style corner_min_wl
source helpers.tcl
source level3.tcl
global_placement
macro_placement -style corner_min_wl -halo {0.5 0.5}

set def_file [make_result_file level3_02.def]
write_def $def_file
diff_file $def_file level3_02.defok
