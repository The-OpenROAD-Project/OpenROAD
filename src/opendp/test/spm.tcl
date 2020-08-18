# spm
source "helpers.tcl"
read_lef sky130/sky130_tech.lef
read_lef spm.lef
read_def spm_replace.def
detailed_placement
check_placement

set def_file [make_result_file spm.def]
write_def $def_file
diff_file $def_file spm.defok
