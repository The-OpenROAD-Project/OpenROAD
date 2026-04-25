# Probe unknown phase error text/handling.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup2.def
read_sdc repair_setup2.sdc
source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5
estimate_parasitics -placement
set status [catch {
  repair_timing -setup -phases "WNS_PATH BAD_PHASE LAST_GASP"
} error]
if { $status == 0 } {
  error "Expected repair_timing to reject BAD_PHASE"
}
puts "Caught expected invalid phase error: $error"
