# hierarchical verilog sample, for dbNetwork pr 2.
source "helpers.tcl"
read_lef example1.lef
read_liberty example1_typ.lib
read_verilog hier2.v
link_design top -hier
set v_file [make_result_file hier2_out.v]
write_verilog $v_file
diff_files $v_file hier2_out.vok

