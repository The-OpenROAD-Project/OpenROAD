# Minimal 2-sink direct fanout (no internal buffers).
# Tests baseline accuracy with the simplest non-trivial Rebuffer scenario.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def report_delay_estimator_rebuffer_minimal.def
create_clock -period 0.3 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

puts "minimal: 2-sink direct, no internal buffers"
report_delay_estimator_accuracy_for_rebuffering \
  -inst r1 \
  -driver_pin r1/Q
