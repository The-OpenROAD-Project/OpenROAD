# Extraction of the experiments' patterns (patterns.def, written by
# make_patterns.py) with the small model: each net against its copy
# rotated by 90 degrees, whose wires all run in their layer's
# non-preferred direction, and a few nets against the field solver.
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_def patterns.def
read_pex_model -itf ../../fsv/test/sky130.itf sky130_coarse.json
extract_pex

proc net_total { name } {
  set net [[ord::get_db_block] findNet $name]
  return [expr { [$net getTotalCapacitance] + [$net getTotalCouplingCap] }]
}

# Every net extracts as its rotated copy does, beyond the order in which
# values are summed.
set differ {}
set pairs 0
foreach net [[ord::get_db_block] getNets] {
  set name [$net getName]
  if { [$net getSigType] != "SIGNAL" || [string match *_rot $name] } {
    continue
  }
  incr pairs
  set drawn [net_total $name]
  set rotated [net_total ${name}_rot]
  if { abs($drawn - $rotated) > 1e-6 * $drawn } {
    lappend differ $name
  }
}
check "every net has a rotated copy" { set pairs } 46
check "rotated copies extract the same" { set differ } {}

compare_pex_fsv -nets {e1_v e2a_v e3_l1_v e4_v e6_v e8_v jog_v} \
  -halo 4 -tol 0.02

exit_summary
