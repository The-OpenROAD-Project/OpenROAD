source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog repair_clk_inverters2_hier.v
link_design top
create_clock -period 1 clk

repair_clock_inverters

set names {}
foreach inst [[ord::get_db_block] getInsts] {
  lappend names [$inst getName]
}
foreach name [lsort $names] {
  puts "instance $name"
}
report_net clk
set verilog_file [make_result_file repair_clk_inverters9_flat_link_out.v]
write_verilog $verilog_file
report_file $verilog_file
