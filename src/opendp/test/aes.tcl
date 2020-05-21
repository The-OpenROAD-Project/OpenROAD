# aes (low utilization)
source helpers.tcl
read_lef Nangate45.lef
read_def aes_cipher_top_replace.def
detailed_placement
check_placement

set def_file [make_result_file aes.def]
write_def $def_file
diff_file aes.defok $def_file
