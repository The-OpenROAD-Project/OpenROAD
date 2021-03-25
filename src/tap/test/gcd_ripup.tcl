source helpers.tcl
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x7.lef
read_def gcd_ripup.def

tapcell -distance "20" -tapcell_master "FILLCELL_X1" -endcap_master "FILLCELL_X1"

tapcell_ripup

set def_file [make_result_file gcd_ripup.def]
write_def $def_file
diff_file $def_file gcd_ripup.defok
