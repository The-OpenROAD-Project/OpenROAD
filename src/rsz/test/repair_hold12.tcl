# repair_timing -hold with hold constraint but no setup constraint
source helpers.tcl
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_hold1.def

create_clock -period 2 clk
# hold constraint but no setup constraint
set_output_delay -clock clk -min -0.3 out
set_propagated_clock clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal1
estimate_parasitics -placement

report_slack out

repair_timing -hold

report_slack out
