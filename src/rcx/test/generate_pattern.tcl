source helpers.tcl

read_lef sky130hs/sky130hs.tlef 

bench_wires -len 100 -all

set def_file [make_result_file generate_pattern.def]
set verilog_file [make_result_file generate_pattern.v]

bench_verilog $verilog_file
write_def $def_file

diff_files generate_pattern.defok $def_file
diff_files generate_pattern.vok $verilog_file
