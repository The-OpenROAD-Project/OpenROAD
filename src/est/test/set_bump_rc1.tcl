# set_bump_rc argument validation
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def reg3.def

# negative values are rejected
catch { set_bump_rc -resistance -1 } msg
puts $msg
# at least one value is required
catch { set_bump_rc } msg
puts $msg
