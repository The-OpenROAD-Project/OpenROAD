# resize to target_slew
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib

read_verilog gain_buffering1.v
link_design top

create_clock -name clk -period 1.0 [get_ports clk]

set_dont_use {sky130_fd_sc_hd__probe_*
    sky130_fd_sc_hd__lpflow_*
    sky130_fd_sc_hd__clkdly*
	sky130_fd_sc_hd__dlygate*
	sky130_fd_sc_hd__dlymetal*
	sky130_fd_sc_hd__clkbuf_*
	sky130_fd_sc_hd__bufbuf_*}

set_dont_touch _53_

report_checks -fields {fanout}
repair_design -buffer_gain 4.0
report_checks -fields {fanout}
