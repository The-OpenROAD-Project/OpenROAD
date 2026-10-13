# -place_ios on nearly empty designs: a handful of tie cells, each driving
# one output port, plus many ports with no connection, in cores of 4 to 14 um.
# Once the cells settle only the IO pins move. The step length used to be
# computed by summing every GCell's movement and subtracting the pins' again
# in float, which is a large number minus itself: it rounded to zero (step
# length 0, the cells stopped moving) or to a small negative number, whose
# square root diverged the placement with GPL-0305. Which core sizes hit it
# depends on rounding, so the test sweeps a range of them.
source helpers.tcl
set test_name place_ios_few_cells
read_lef Nangate45/Nangate45.lef

set db [ord::get_db]
set tech [ord::get_db_tech]
set chip [odb::dbChip_create $db $tech]
set dbu [$tech getDbUnitsPerMicron]
set site [[lindex [$db getLibs] 0] findSite "FreePDK45_38x28_10R_NP_162NW_34O"]
set tie [$db findMaster "LOGIC0_X1"]

# num_cells tie cells driving one output port each, num_unconnected input
# ports with nothing on their nets, rows over a core_um square core with a
# 5 um margin to the die.
proc make_design { core_um { num_cells 8 } { num_unconnected 24 } } {
  global chip dbu site tie
  set block [odb::dbBlock_create $chip "few_cells_$core_um"]
  for { set i 0 } { $i < $num_cells } { incr i } {
    set inst [odb::dbInst_create $block $tie "tie_$i"]
    set net [odb::dbNet_create $block "out_$i"]
    [$inst findITerm "Z"] connect $net
    set bterm [odb::dbBTerm_create $net "out_$i"]
    $bterm setIoType OUTPUT
  }
  for { set i 0 } { $i < $num_unconnected } { incr i } {
    set net [odb::dbNet_create $block "in_$i"]
    set bterm [odb::dbBTerm_create $net "in_$i"]
    $bterm setIoType INPUT
  }

  set die_rect [odb::Rect]
  $die_rect init 0 0 [expr { ($core_um + 10) * $dbu }] \
    [expr { ($core_um + 10) * $dbu }]
  $block setDieArea $die_rect
  set site_w [$site getWidth]
  set site_h [$site getHeight]
  set num_sites [expr { $core_um * $dbu / $site_w }]
  set num_rows [expr { $core_um * $dbu / $site_h }]
  for { set r 0 } { $r < $num_rows } { incr r } {
    odb::dbRow_create $block "row_$r" $site [expr { 5 * $dbu }] \
      [expr { 5 * $dbu + $r * $site_h }] [expr { $r % 2 ? "MX" : "R0" }] \
      HORIZONTAL $num_sites $site_w
  }
  set core_rect [odb::Rect]
  $core_rect init [expr { 5 * $dbu }] [expr { 5 * $dbu }] \
    [expr { 5 * $dbu + $num_sites * $site_w }] \
    [expr { 5 * $dbu + $num_rows * $site_h }]
  $block setCoreArea $core_rect
  make_tracks
  return $block
}

for { set core_um 4 } { $core_um <= 14 } { incr core_um } {
  set block [make_design $core_um]

  set failed [catch { global_placement -place_ios } msg]
  check "$core_um um core: global_placement -place_ios completes" \
    { set failed } 0
  if { $failed } {
    puts $msg
  }

  set core [$block getCoreArea]
  foreach inst [$block getInsts] {
    set name [$inst getName]
    check "$core_um um core: $name is placed" { $inst isPlaced } 1
    set box [$inst getBBox]
    check "$core_um um core: $name is inside the core" {
      expr {
        [$box xMin] >= [$core xMin] && [$box xMax] <= [$core xMax]
        && [$box yMin] >= [$core yMin] && [$box yMax] <= [$core yMax]
      }
    } 1
  }

  set die [$block getDieArea]
  foreach bterm [$block getBTerms] {
    set name [$bterm getName]
    set bpins [$bterm getBPins]
    check "$core_um um core: $name has one pin" { llength $bpins } 1
    check "$core_um um core: $name is placed" \
      { [lindex $bpins 0] getPlacementStatus } PLACED
    set box [$bterm getBBox]
    set cx [expr { ([$box xMin] + [$box xMax]) / 2 }]
    set cy [expr { ([$box yMin] + [$box yMax]) / 2 }]
    check "$core_um um core: $name is on the die boundary" {
      expr {
        $cx == [$die xMin] || $cx == [$die xMax]
        || $cy == [$die yMin] || $cy == [$die yMax]
      }
    } 1
  }

  odb::dbBlock_destroy $block
}

# A design that is genuinely infeasible for -place_ios is still refused,
# with its own message: no perimeter left for the pins.
set block [make_design 8]
exclude_io_pin_region -region bottom:* -region left:* -region right:* \
  -region top:*
set failed [catch { global_placement -place_ios } msg]
check "the whole perimeter excluded is refused" { set failed } 1
check "the refusal names GPL-0180" { string match {*GPL-0180*} $msg } 1

exit_summary
