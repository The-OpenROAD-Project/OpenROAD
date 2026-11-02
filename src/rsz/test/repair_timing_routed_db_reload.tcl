# Fresh session for repair_timing_routed_db.tcl: read its routed database
# and repair timing with the placement parasitics estimated here.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_db [make_result_file repair_timing_routed_db.odb]

# gcd_nangate45.sdc with a period tight enough for setup violations.
create_clock [get_ports clk] -name core_clock -period 0.6
set_max_fanout 100 [current_design]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

check "the database reads back routed" { design_is_routed } 1
repair_timing -setup
check "repair_timing keeps the estimated parasitics" {
  est::parasitics_source
} placement
report_worst_slack -max -digits 3
exit_summary
