source helpers.tcl

read_lef sky130hs/sky130hs.tlef 

read_def generate_pattern.defok
bench_read_spef generate_pattern.spefok

set rules_file [make_result_file patterns.rules]
write_rules -file $rules_file -db

exec rm rulesGen.log

diff_files patterns.rulesok $rules_file
