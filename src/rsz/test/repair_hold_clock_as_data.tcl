# OpenROAD issue #8075: repair_timing -hold must detect hold violations on
# register data (D) pins that sample a signal sourced from a clock, i.e. a
# clock used as data crossing into a different sampling-clock domain.
#
# Here clk1 is used BOTH as a clock (for r1) and as data into r2/D (r2 is
# clocked by clk2).  Because clk1 is a clock, r2/D lands in clk1's clock
# network and isClock(r2/D) is true -- yet r2/D carries a genuine hold check
# from clk2.  Before the fix, findHoldViolations skipped this endpoint:
# repair_timing -hold reported "No hold violations found" while a real
# cross-clock-domain hold violation (negative min slack) remained on r2/D.
#
# This test fails on the unfixed code (hold violation silently missed,
# slack stays negative) and passes after the fix (violation detected and
# repaired to non-negative slack).
source helpers.tcl
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_hold_clock_as_data.def

create_clock -period 2 -name clk1 [get_ports clk1]
create_clock -period 2 -name clk2 [get_ports clk2]
set_propagated_clock [all_clocks]

# Make the capture clock (clk2) arrive much later than the launch (clk1) so
# the very short clk1->dbuf->r2/D data path arrives far too early for clk2:
# a real hold violation on r2/D.
set_clock_latency 0.6 [get_clocks clk2]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal1
estimate_parasitics -placement

puts "=== r2/D hold slack (min) before repair: real cross-domain violation ==="
report_slack r2/D

repair_timing -hold

puts "=== r2/D hold slack (min) after repair: must be detected and fixed ==="
report_slack r2/D
