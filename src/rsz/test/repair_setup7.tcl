# repair_timing -setup fail early on hopeless situation
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup1.def
create_clock -period 0.03 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

catch {repair_timing -setup -verbose} error
puts $error

