set test_case v12_patterns
set test_dir ../../../

read_lef $test_dir/sky130hs/sky130hs.tlef

bench_wires -len 100 -all -v1

bench_verilog $test_case.v
write_def $test_case.def
