# DEF units that do not evenly divide the database units per micron must be
# rejected rather than scaling every coordinate by a truncated factor.
source "helpers.tcl"

read_lef "Nangate45/Nangate45.lef"

catch { read_def "read_def_units_not_divisor.def" } msg
puts $msg
