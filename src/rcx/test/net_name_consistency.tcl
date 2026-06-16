# Tests a net name consistency b/w Verilog and SPEF.
source helpers.tcl
source Nangate45/Nangate45.vars

set test_name net_name_consistency

proc write_spef_for_diff { input_file output_file } {
  set input [open $input_file r]
  set output [open $output_file w]
  set line_number 1

  while { [gets $input line] >= 0 } {
    if { $line_number >= 10 } {
      puts $output [string trimright $line]
    }
    incr line_number
  }

  close $input
  close $output
}

read_lef $tech_lef
read_lef $std_cell_lef
read_liberty $liberty_file

read_verilog $test_name.v
link_design -hier top
read_def -floorplan_initialize $test_name.def

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
write_spef_for_diff $spef_file $filtered_spef_file
set filtered_spefok_file [make_result_file $test_name.spefok.tmp]
write_spef_for_diff $test_name.spefok $filtered_spefok_file
diff_files $filtered_spefok_file $filtered_spef_file

# Test reading the spef for no errors in the .ok
read_spef $spef_file
