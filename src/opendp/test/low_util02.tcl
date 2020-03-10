source helpers.tcl
read_lef Nangate45.lef
read_def aes_cipher_top_replace.def
detailed_placement
check_placement

set def_file [make_result_file low_util02.def]
write_def $def_file
diff_file $def_file low_util02.defok
