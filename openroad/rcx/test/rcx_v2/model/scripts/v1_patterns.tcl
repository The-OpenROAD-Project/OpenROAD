set test_case v1_patterns
set test_dir ../../../

read_lef $test_dir/sky130hs/sky130hs.tlef

bench_wires -len 100 -all

bench_verilog $test_case.v
write_def $test_case.def
