source helpers.tcl
read_liberty ./library/nangate45/NangateOpenCellLibrary_typical.lib

read_lef ./nangate45.lef
read_def ./simple01-td.def

set_wire_rc -signal -layer metal3

# No clock defined: virtual CTS warns and is skipped (GPL-0160).
global_placement -timing_driven -virtual_cts
