# PARALLEL_EVAL undoes batches that cross -max_utilization and warns once.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup_path11.def
read_sdc repair_setup_path11.sdc

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# Every move adds area here, so a limit just above the current utilization
# makes each batch cross it.
set max_util [expr { 100.0 * [rsz::utilization] * 1.001 }]
puts [format "max_utilization %.4f%%" $max_util]
report_design_area

repair_timing -setup -phases "PARALLEL_EVAL" -max_iterations 3 -verbose \
  -max_utilization $max_util

# The undone batches leave the design below the limit.
report_design_area
puts [format "utilization %.4f%%" [expr { 100.0 * [rsz::utilization] }]]
