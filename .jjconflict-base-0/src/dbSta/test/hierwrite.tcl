source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_db hierwrite.odb -hier
set v_file [make_result_file hierwrite.v]
write_verilog $v_file
diff_files $v_file hierwrite.vok
