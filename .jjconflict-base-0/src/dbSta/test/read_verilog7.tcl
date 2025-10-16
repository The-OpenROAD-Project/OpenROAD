# verilog constants 1'b0/1'b1
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog read_verilog7.v
link_design top

report_constant u1/A2

set def_file [make_result_file read_verilog7.def]
write_def $def_file
diff_files $def_file read_verilog7.defok
