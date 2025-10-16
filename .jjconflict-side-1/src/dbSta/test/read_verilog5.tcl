# input tri port
source "helpers.tcl"
read_liberty bidir1.lib
read_lef bidir1.lef
read_verilog read_verilog5.v
link_design top

set def_file [make_result_file read_verilog5.def]
write_def $def_file
diff_files $def_file read_verilog5.defok
