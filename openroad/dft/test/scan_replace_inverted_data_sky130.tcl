# The scan cell stores the inverted data (test_cell next_state "!D"), so its
# first state variable is not the non-scan Q. scan_replace must not map Q onto
# Q_N; it rejects the scan cell.
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty scan_replace_inverted_data_sky130.lib

read_def scan_replace_inverted_data_sky130.def

create_clock -name main_clock -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock}]

report_instance ff1
scan_replace
report_instance ff1
