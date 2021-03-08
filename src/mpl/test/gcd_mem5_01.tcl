# gcd 5 macros halo=0.5
source helpers.tcl

read_liberty Nangate45/Nangate45_typ.lib
read_liberty Nangate45/fakeram45_64x7.lib

read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x7.lef

read_def gcd_mem5.def
read_sdc gcd.sdc

macro_placement -halo {0.5 0.5}

set def_file [make_result_file gcd_mem5_01.def]
write_def $def_file
diff_file $def_file gcd_mem5_01.defok
