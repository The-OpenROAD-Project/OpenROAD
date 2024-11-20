set test_case under_v2_bench
set test_dir ../../../

read_lef $test_dir/sky130hs/sky130hs.tlef

bench_wires_gen -dbg 2 -under

bench_verilog $test_case.verilog
write_def $test_case.def
