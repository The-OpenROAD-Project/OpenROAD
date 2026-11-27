# scan_replace maps Q to the scan cell's Q_N and Q_N to Q, and moves the
# instance and timing arc disables on those ports to the mapped ports
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty scan_replace_renamed_port_sky130.lib
read_verilog scan_replace_renamed_port_sky130.v
link_design top

create_clock -name main_clock -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock}]
set_disable_timing -from CLK -to Q [get_cells ff1]
set_disable_timing [get_timing_edges -from [get_pins ff1/CLK] -to [get_pins ff1/Q_N]]

scan_replace
report_instance ff1

set sdc_file [make_result_file scan_replace_renamed_port_sky130.sdc]
write_sdc -no_timestamp $sdc_file
report_file $sdc_file
