# port assigned to net (port with alias)
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog read_verilog8.v
link_design top

set def_file [make_result_file read_verilog8.def]
write_def $def_file
diff_files $def_file read_verilog8.defok
