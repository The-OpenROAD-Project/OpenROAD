# hierarchical verilog sample, for dbNetwork pr 2.
source "helpers.tcl"
read_lef example1.lef
read_liberty example1_typ.lib
read_verilog hier2.v
link_design top -hier
write_verilog hier2_out.v
diff_files hier2_out.v hier2_out.vok

