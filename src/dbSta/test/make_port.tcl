# make_port succeeds
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog make_port.v
link_design testcase

make_port dummy_port input

set v_file [make_result_file "make_port.v"]
write_verilog $v_file
diff_files $v_file "make_port.vok"
