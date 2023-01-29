source helpers.tcl

read_lef sky130hs/sky130hs.tlef 

read_def generate_pattern.defok
# generate_pattern.spef needs to be generated from an OSS tool (eg magic)
bench_read_spef generate_pattern.spef

set rules_file [make_result_file patterns.rules]
write_rules -file $rules_file

exec rm rulesGen.log

diff_files patterns.rulesok $rules_file
