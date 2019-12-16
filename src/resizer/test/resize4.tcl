# resize -dont_use
source helpers.tcl
read_liberty nlc18.lib
read_lef nlc18.lef
read_def reg1.def

create_clock -name clk -period 1 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}
# no placement, so add loads
set_load .2 u1z
set_load .2 r1q

report_checks
resize -resize -dont_use {*/*8 */*4}
report_checks

set def_file [make_result_file resize4.def]
write_def $def_file
diff_files $def_file resize4.defok
