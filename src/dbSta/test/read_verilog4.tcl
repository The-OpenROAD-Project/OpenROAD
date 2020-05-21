# read_verilog link error
source "helpers.tcl"
read_lef liberty1.lef
read_verilog read_verilog4.v
link_design top

set def_file [make_result_file read_verilog4.def]
write_def $def_file
diff_files $def_file read_verilog4.defok
