# repair_hold_violations
source helpers.tcl
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_hold1.def

create_clock -period 2 clk
set_input_delay -clock clk 0 {in1 in2}
set_output_delay -clock clk -0.1 out
set_propagated_clock clk

set_wire_rc -layer metal1
estimate_parasitics -placement

report_checks -path_delay min -format full_clock -unique_paths_to_endpoint \
  -endpoint_count 5

repair_timing -hold

report_checks -path_delay min -format full_clock -unique_paths_to_endpoint \
  -endpoint_count 5
