# LegacyMt policy smoke on a small setup-repair case
source "helpers.tcl"

#set ::env(RSZ_POLICY) legacy
set ::env(RSZ_POLICY) legacy_mt

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup1.def
create_clock -period 0.3 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

repair_timing -setup

puts "pass"

unset -nocomplain ::env(RSZ_POLICY)
