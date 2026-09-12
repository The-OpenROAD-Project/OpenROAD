# Placement must validate names before even temporary candidate swaps.
source helpers.tcl
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_placed.def
create_clock -name core_clock -period 2 [get_ports clk]
set_wire_rc -signal -layer metal3
estimate_parasitics -placement
proc placement_snapshot { } {
  set result [dict create]
  foreach inst [[ord::get_db_block] getInsts] {
    dict set result [$inst getId] \
      [list [$inst getLocation] [$inst getOrient] [$inst getPlacementStatus]]
  }
  return $result
}
proc contents { path } {
  set stream [open $path r]
  set result [read $stream]
  close $stream
  return $result
}
set block [ord::get_db_block]
# Renaming the last movable cell checks preflight even after valid candidates.
set candidate NULL
foreach inst [$block getInsts] {
  if { ![$inst isFixed] && ![$inst isDoNotTouch] } {
    set candidate $inst
  }
}
set original_name [$candidate getName]
set key 0011223344556677889900aabbccddeeff00112233445566778899aabbccddee
set claims [make_result_file place_claim_names.csv]
foreach unsupported { zzcell,b "zzcell\nnext" " zzcell" "zzcell " } {
  $candidate rename $unsupported
  set before [placement_snapshot]
  set stream [open $claims w]
  puts -nonewline $stream "previous claim file"
  close $stream
  set failed [catch {
    place_watermark -key_hex $key -claims_file $claims \
      -hpwl_eps_um 1 -pair_dist_um 3 -pairs_per_tile 64
  } message]
  check "unsupported name fails explicitly" { set failed } 1
  check "the error identifies the claim format" { string match {*WMK-0112*} $message } 1
  check "all placements are preserved" { placement_snapshot } $before
  check "the existing claim file is preserved" { contents $claims } "previous claim file"
}
$candidate rename $original_name
check "supported names can still be embedded" {
  place_watermark -key_hex $key -claims_file $claims \
    -hpwl_eps_um 1 -pair_dist_um 3 -pairs_per_tile 64
} 24
check_placement
exit_summary
