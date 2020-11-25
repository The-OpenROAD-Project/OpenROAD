source helpers.tcl
read_lef nangate45-bench/tech/NangateOpenCellLibrary.lef
read_lef nangate45-bench/tech/fakeram45_64x7.lef

read_def gcd_fakeram.def

set def_file [make_result_file gcd_fakeram.def]

tapcell -endcap_cpp "2" -distance "20" -tapcell_master "FILLCELL_X1" -endcap_master "FILLCELL_X1"

write_def $def_file
diff_file $def_file gcd_fakeram.defok
