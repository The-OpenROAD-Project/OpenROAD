source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef Nangate45/fakeram45_64x7.lef
read_def cut_rows_short_core.def

# A minimum row height taller than the core must not turn the whole core into
# a narrow region and cut away every row. See issue #11223.
cut_rows -endcap_master "TAPCELL_X1" -row_min_height 60

set def_file [make_result_file cut_rows_short_core.def]
write_def $def_file
diff_file cut_rows_short_core.defok $def_file
