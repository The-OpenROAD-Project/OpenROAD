# resize reg1 (no placement)
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def reg2.def

create_clock -name clk -period 1 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}
# no placement, so add loads
set_load 20 r1q
set_load 20 r2q
set_load 20 u1z
set_load 20 u2z

report_checks
resize
report_checks

set def_file [make_result_file resize2.def]
write_def $def_file
diff_files resize2.defok $def_file
