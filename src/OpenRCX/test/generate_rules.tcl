source helpers.tcl

read_lef sky130/sky130_tech.lef 

read_def generate_pattern.defok
bench_read_spef generate_pattern.spefok

set rules_file [make_result_file patterns.rules]
write_rules -file $rules_file -db

exec rm rulesGen.log

diff_files patterns.rulesok $rules_file
