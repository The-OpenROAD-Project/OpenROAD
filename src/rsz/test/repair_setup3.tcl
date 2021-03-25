# repair_timing -setup thru latch
source helpers.tcl
read_liberty sky130hd/sky130hd_tt.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_def repair_setup3.def

create_clock -period 1 clk
set_wire_rc -layer met1
estimate_parasitics -placement
# force violation at r3/D to repair
set_load 1.0 l2q

repair_timing -setup
