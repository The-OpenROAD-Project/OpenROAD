# scan_replace keeps the SDC constraints on the replaced flop's pins and
# instance
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_def one_cell_sky130.def

create_clock -name main_clock -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock}]
set_false_path -from [get_pins ff1/CLK] -to [get_ports output1]
set_multicycle_path 2 -setup -to [get_pins ff1/D]
set_multicycle_path 3 -hold -from [get_cells ff1]
set_case_analysis 1 [get_pins ff1/SET_B]
set_clock_latency 0.1 [get_pins ff1/CLK]
set_clock_uncertainty 0.05 [get_pins ff1/CLK]
set_min_pulse_width 0.3 [get_pins ff1/CLK]
set_max_capacitance 0.2 [get_pins ff1/Q]
set_disable_timing -from SET_B -to Q [get_cells ff1]
set_disable_timing [get_timing_edges -from [get_ports set_b] -to [get_pins ff1/SET_B]]
set_timing_derate -cell_delay -late 1.1 [get_cells ff1]
set_input_delay 0.2 -clock main_clock [get_ports port1]

# The moved constraints also apply to timing
proc report_timing_summary { } {
  report_checks -path_delay min_max -format end
  report_checks -from [get_pins ff1/CLK] -to [get_ports output1] -format end
}
report_timing_summary

set before_file [make_result_file scan_replace_sdc_sky130_before.sdc]
write_sdc -no_timestamp $before_file

scan_replace
report_instance ff1
report_timing_summary

set after_file [make_result_file scan_replace_sdc_sky130_after.sdc]
write_sdc -no_timestamp $after_file
report_file $after_file
diff_files $before_file $after_file
