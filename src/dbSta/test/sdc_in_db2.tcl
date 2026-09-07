# read_db -sdc restores the constraints stored in the .odb. No .sdc file is
# read here: the design arrives constrained.
source "sdc_in_db_common.tcl"
load_libs
read_db -sdc sdc_in_db.odb

check "stored form" { ord::sdc_in_db_kind } native
check "clocks restored" { lsort [lmap clk [all_clocks] { get_name $clk }] } {clk1 clk2}
check "clk1 period" { get_property [lindex [get_clocks clk1] 0] period } 10
check "clk2 period" { get_property [lindex [get_clocks clk2] 0] period } 20
set sdc [make_result_file sdc_in_db2.sdc]
write_sdc -no_timestamp $sdc
set in_delay {set_input_delay 10.0000 -clock \[get_clocks \{clk2\}\]}
append in_delay { -add_delay \[get_ports \{in1\}\]}
check "input delay restored" { file_matches $sdc $in_delay } 1
set out_delay {set_output_delay 10.0000 -clock \[get_clocks \{clk2\}\]}
append out_delay { -add_delay \[get_ports \{out\}\]}
check "output delay restored" { file_matches $sdc $out_delay } 1
exit_summary
