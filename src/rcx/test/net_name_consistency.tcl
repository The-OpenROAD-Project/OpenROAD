# Tests a net name consistency b/w Verilog and SPEF.
source helpers.tcl
source Nangate45/Nangate45.vars

set test_name net_name_consistency

read_lef $tech_lef
read_lef $std_cell_lef
read_liberty $liberty_file

read_def $test_name.def

# Extract RC from the routed DEF fixture.
define_process_corner -ext_model_index 0 X
extract_parasitics -ext_model_file $rcx_rules_file

# Write Verilog
set verilog_file [make_result_file $test_name.v]
write_verilog $verilog_file
diff_files $test_name.vok $verilog_file

# Write SPEF
set spef_file [make_result_file $test_name.spef]
write_spef $spef_file

# Filter out the date and software version lines before diff_files
set filtered_spef_file [make_result_file $test_name.spef.tmp]
exec tail -n +10 $spef_file > $filtered_spef_file
set filtered_spefok_file [make_result_file $test_name.spefok.tmp]
exec tail -n +10 $test_name.spefok > $filtered_spefok_file
diff_files $filtered_spefok_file $filtered_spef_file

# Test reading the spef for no errors in the .ok
read_spef $spef_file
