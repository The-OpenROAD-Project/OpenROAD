source helpers.tcl

read_lef sky130/sky130_tech.lef 

bench_wires -len 100 -all

set def_file [make_result_file generate_pattern.def]
set verilog_file [make_result_file generate_pattern.v]

bench_verilog $verilog_file
write_def $def_file

diff_files generate_pattern.defok $def_file
diff_files generate_pattern.vok $verilog_file
