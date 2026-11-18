# global_route guides estimate parasitics on a detail-routed design;
# detailed_route guides do not (issue #11852).
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_db "sky130hs/gcd_grt.db"
source "sky130hs/sky130hs.rc"
set_wire_rc -signal -layer met2
set_wire_rc -clock -layer met5

# Make the design read as detail-routed, keeping the global_route guides.
set layer [[ord::get_db_tech] findLayer met1]
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
check "the design is detail-routed" { design_is_routed } 1
check "global_route guides give global routes" { grt::have_routes } 1
check "global_route guides estimate on a detail-routed design" {
  catch { estimate_parasitics -global_routing }
} 0
check "the parasitics come from global routing" {
  est::parasitics_source
} global_routing

# Mark the guides as detailed_route -save_guide_updates does.
foreach net [[ord::get_db_block] getNets] {
  foreach guide [$net getGuides] {
    $guide setIsFromDetailedRoute 1
  }
}
write_db [make_result_file estimate_parasitics_droute_guides.odb]

puts [exec [info nameofexecutable] -no_splash -no_init -exit \
  -threads [thread_count] \
  [file join [file dirname [info script]] \
    "estimate_parasitics_droute_guides_reload.tcl"] 2>@1]

exit_summary
