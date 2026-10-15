source helpers.tcl
read_liberty ./library/nangate45/NangateOpenCellLibrary_typical.lib

read_lef ./nangate45.lef
read_def ./simple01-td.def

create_clock -name core_clock -period 2 clk

set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5

# An out-of-range skew fraction is rejected (GPL-0165), from Tcl as from
# Python and C++.
catch { global_placement -timing_driven -virtual_cts -virtual_cts_max_skew_fraction 2.0 } msg
puts $msg

global_placement -timing_driven -virtual_cts -virtual_cts_max_skew_fraction 1.0

estimate_parasitics -placement
report_worst_slack
