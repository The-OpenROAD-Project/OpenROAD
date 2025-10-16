# repair_design shorted outputs (synthesized RAM)
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_def repair_design2.def

source sky130hd/sky130hd.rc
set_wire_rc -layer met2
estimate_parasitics -placement
# force a max cap violation at the shorted tristate net
set_load 1 n1
repair_design
