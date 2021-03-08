# -snap_layer 3
source helpers.tcl

read_liberty Nangate45/Nangate45_typ.lib
read_liberty Nangate45/fakeram45_64x7.lib

read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x7.lef

read_def east_west1.def
read_sdc gcd.sdc

global_placement -skip_initial_place
macro_placement -halo {1.0 1.0} -snap_layer 3

set def_file [make_result_file snap_layer1.def]
write_def $def_file
diff_file $def_file snap_layer1.defok
