# optimize_power on a SINGLE-Vt library (Nangate45): there are no lower-leakage
# same-footprint variants, so the design must be left unchanged. Also proves
# the command is safe / a no-op when no VT variants exist.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup_size_down_fanout.def
create_clock -period 0.35 clk
set_load 1.0 [all_outputs]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# Snapshot every instance's master before.
set before {}
foreach inst [get_cells *] {
  lappend before "[get_name $inst] [get_property $inst ref_name]"
}

set changed [optimize_power -leakage]

set after {}
foreach inst [get_cells *] {
  lappend after "[get_name $inst] [get_property $inst ref_name]"
}

if { $changed == 0 && $before eq $after } {
  puts "SINGLE-VT: design unchanged (PASS)"
} else {
  puts "SINGLE-VT: design changed (FAIL) changed=$changed"
}
