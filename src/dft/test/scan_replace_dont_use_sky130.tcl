source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_def one_cell_sky130.def

create_clock -name main_clock -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock}]

# Mark the scan cell master that scan_replace prefers by default as dont_use
# to verify that a different master is inserted instead.
set_dont_use {sky130_fd_sc_hd__sdfstp_1}

report_instance ff1
scan_replace
report_instance ff1
