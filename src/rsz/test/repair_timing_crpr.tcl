# repair_timing CRPR setup/hold UI options.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_hold1.def

create_clock -period 0.08 clk
set_input_delay -clock clk 0.0 {in1 in2}
set_output_delay -clock clk -0.3 out
set_propagated_clock clk
set_operating_conditions -analysis_type on_chip_variation
set_timing_derate -early 0.9
set_timing_derate -late 1.1

source Nangate45/Nangate45.rc
set_wire_rc -layer metal1
estimate_parasitics -placement

report_worst_slack -max
repair_timing -setup -skip_crpr_setup -max_iterations 1
report_worst_slack -max
report_tns -max -digits 3

repair_timing -hold -skip_crpr_hold -max_iterations 1
report_worst_slack -min
report_tns -min -digits 3
