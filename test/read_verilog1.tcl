# reg1
source "helpers.tcl"
read_lef liberty1.lef
read_verilog reg1.v
link_design top

set def_file [make_result_file read_verilog1.def]
write_def $def_file
diff_files $def_file read_verilog1.defok
