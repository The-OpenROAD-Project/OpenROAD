# mem0 with west connctions, mem1 with east connections
source helpers.tcl

read_liberty Nangate45/Nangate45_typ.lib
read_liberty Nangate45/fakeram45_64x7.lib

read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x7.lef

read_def east_west1.def
read_sdc gcd.sdc

global_placement -skip_initial_place
macro_placement -style corner_max_wl -halo {0.5 0.5}
# corner_max_wl cannot get this right
#macro_placement -style center_spread -halo {0.5 0.5}

set def_file [make_result_file east_west1.def]
write_def $def_file
diff_file $def_file east_west1.defok
