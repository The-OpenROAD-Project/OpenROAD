source "helpers.tcl"
read_lef merged_spacing.lef
read_def sw130_random.def

set rpt_file [make_result_file "sw130_random_simple.rpt"]
check_antennas -report_file $rpt_file -report_violating_nets
report_file $rpt_file
