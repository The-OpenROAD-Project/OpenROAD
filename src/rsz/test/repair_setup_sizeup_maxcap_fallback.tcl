# Test SizeUpGenerator max-cap fallback: with a tight max-cap constraint
# on the fanin net, the resizer should still upsize the target cell to a
# size that fits within the constraint rather than giving up.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup_sizeup_maxcap_fallback.def
create_clock -period 0.15 clk
set_load 20.0 [get_nets n2]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# Tight max-cap on the fanin net limits how much u_target can grow.
set_max_capacitance 140.5 [get_pins u_upstream/Z]

set_dont_use [get_lib_cells CLKBUF*]
set_dont_touch [get_cells u_upstream]
set_dont_touch [get_cells {u_side*}]
set_dont_touch [get_cells r1]
set_dont_touch [get_cells r2]

report_checks -fields input -digits 3

repair_timing -setup -sequence "sizeup" -skip_last_gasp

report_checks -fields input -digits 3
