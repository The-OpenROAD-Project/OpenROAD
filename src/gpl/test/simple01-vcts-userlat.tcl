source helpers.tcl
read_liberty ./library/nangate45/NangateOpenCellLibrary_typical.lib

read_lef ./nangate45.lef
read_def ./simple01-td.def

create_clock -name core_clock -period 2 clk

set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5

# Regression for virtual CTS clobbering user clock-latency constraints.
#
# Virtual CTS temporarily sets a per-pin network latency on every register
# clock sink, then removes it before final timing. The removal deletes the
# entry entirely (STA has no restore), so if virtual CTS were allowed to
# write over a pin the user already constrained with set_clock_latency, that
# user constraint would be silently deleted when virtual CTS cleans up.
#
# Here we constrain exactly one of the 34 register clock pins. virtual CTS
# must leave that pin alone and only annotate the other 33. The check is the
# GPL-0162 count: it reports "set 33 ..." (not 34). A regression that resumes
# overwriting user-constrained pins would report 34 and delete the user's
# 0.5ns latency on _569_/CK.
set_clock_latency 0.5 [get_pins _569_/CK]

global_placement -timing_driven -virtual_cts

estimate_parasitics -placement
report_worst_slack
