# repair_timing -hold
source helpers.tcl
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_hold1.def

create_clock -period 2 clk
set_input_delay -clock clk 0.0 {in1 in2}
set_output_delay -clock clk -0.3 out
set_propagated_clock clk

set_wire_rc -layer metal1
estimate_parasitics -placement

report_checks -path_delay min -to r2/D
report_checks -path_delay min -to r3/D
report_checks -path_delay min -to out

repair_timing -hold

report_checks -path_delay min

# Verilog "ports" are based on net names so make sure port nets
# are preserved on inputs and outputs.
report_net -connections -verbose in2
report_net -connections -verbose out
