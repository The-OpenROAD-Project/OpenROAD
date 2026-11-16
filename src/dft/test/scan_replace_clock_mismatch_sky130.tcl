# The scan cell matches the non-scan cell except that it captures on !CLK.
# scan_replace must not replace the positive-edge flop with it.
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty scan_replace_clock_mismatch_sky130.lib

read_def one_cell_reset_sky130.def

create_clock -name main_clock -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock}]

report_instance ff1
scan_replace
report_instance ff1
