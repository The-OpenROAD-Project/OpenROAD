include "helpers.tcl"

set test_name emap_ibex_sky130hd

read_liberty sky130/sky130_fd_sc_hd__ss_n40C_1v40.lib
read_lef sky130/sky130hd.tlef
read_lef sky130/sky130hd_std_cell.lef

read_verilog ibex_sky130hd.v
link_design ibex_core
read_sdc ibex_sky130hd.sdc

source sky130/sky130hd.rc

puts "-- Before --\n"
report_cell_usage
report_timing_histogram
report_checks
report_wns
report_tns

write_verilog_for_eqy $test_name before "None"

puts "-- After --\n"

resynth_emap \
  -map_multioutput

repair_timing

report_cell_usage
report_timing_histogram
report_checks
report_wns
report_tns

run_equivalence_test $test_name \
  -liberty_files sky130hd/sky130_fd_sc_hd__ss_n40C_1v40.lib \

