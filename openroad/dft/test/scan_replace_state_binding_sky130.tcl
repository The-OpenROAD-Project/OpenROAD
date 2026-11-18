# The scan cell orders its ff state variables as ff("IQ_N","IQ") while the
# non-scan cell uses ff("IQ","IQ_N"). The names are local, so scan_replace must
# keep the net on the non-inverted output Q rather than moving it to Q_N.
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty scan_replace_state_binding_sky130.lib

read_def one_cell_reset_sky130.def

create_clock -name main_clock -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock}]

report_instance ff1
scan_replace
report_instance ff1
