# repair_timing on a routed database read in a fresh session keeps the
# parasitics the flow estimated instead of switching to detailed_routing
# parasitics without usable global routes (issue #11852, RSZ-0074).
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_placed.def
make_tracks

# Give every signal net a wire so the database reads back as detail-routed;
# repair_timing only looks at design_is_routed, not at the wires themselves.
set layer [[ord::get_db_tech] findLayer metal1]
foreach net [[ord::get_db_block] getNets] {
  if { [$net isSpecial] } {
    continue
  }
  set encoder [odb::dbWireEncoder]
  $encoder begin [odb::dbWire_create $net]
  $encoder newPath $layer "ROUTED"
  $encoder addPoint 2000 2000
  $encoder addPoint 4000 2000
  $encoder end
}

check "the design is routed" { design_is_routed } 1
write_db [make_result_file repair_timing_routed_db.odb]

# Repair timing in a separate process so no global routes are left over.
puts [exec [info nameofexecutable] -no_splash -no_init -exit \
  -threads [thread_count] \
  [file join [file dirname [info script]] "repair_timing_routed_db_reload.tcl"] 2>@1]
