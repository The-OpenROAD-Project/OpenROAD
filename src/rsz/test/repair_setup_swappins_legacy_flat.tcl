# Legacy SwapPinsMove parity test on a flat placed combinational path.
source "helpers.tcl"
source Nangate45/Nangate45.vars
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup_swappins_legacy_flat.def

create_clock [get_ports clk] -period 0.1
set_clock_uncertainty 0 [get_clocks clk]
set_input_delay -clock clk 0.02 [get_ports a1]
set_input_delay -clock clk 0.02 [get_ports a2]
set_input_delay -clock clk 0.02 [get_ports a3]
set_input_delay -clock clk 0.00 [get_ports a4]
set_input_delay -clock clk 0.00 [get_ports a5]
set_input_delay -clock clk 0.00 [get_ports a6]
set_output_delay -clock clk 0.01 [get_ports y1]
set_output_delay -clock clk 0.01 [get_ports y2]

source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5
estimate_parasitics -placement
report_worst_slack
repair_timing -setup -sequence "swap" -skip_last_gasp
report_worst_slack
report_checks -fields input -digits 3
