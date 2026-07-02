# Regression for issue #10490: a macro PDN grid must not cause pad direct
# connections to extend into the core.
#
# When pad direct-connect is enabled at the grid level
# (define_pdn_grid -connect_to_pads), a pad connection snaps to the closest
# STRIPE/RING target on a connectable layer.  If a macro (instance) PDN grid is
# built first, its in-core stripes become visible target candidates and -- when
# there is no core target on the pad-connect layer near the pad -- the pad
# connection used to snap to those macro stripes, dragging the connection deep
# into the core.
#
# The fix makes PadDirectConnectionStraps::isTargetShape() reject shapes owned
# by an instance grid, so pad connections only land on core/existing grid
# shapes.  This test asserts that no pad connection extends past the core
# boundary.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_bsg_black_parrot/dummy_pads.lef
read_lef Nangate45/fakeram45_64x32.lef

read_def nangate_bsg_black_parrot/floorplan.def

set block [ord::get_db_block]
set db [ord::get_db]

# Place an SRAM macro just inside the core, above the south VDD/VSS pads.
set master [$db findMaster fakeram45_64x32]
set inst [odb::dbInst_create $block $master "repro_sram"]
$inst setOrigin 850000 450000
$inst setPlacementStatus FIRM
foreach iterm [$inst getITerms] {
  set name [[$iterm getMTerm] getName]
  if { [string match "*VDD*" $name] } {
    odb::dbITerm_connect $iterm [$block findNet VDD]
  } elseif { [string match "*VSS*" $name] } {
    odb::dbITerm_connect $iterm [$block findNet VSS]
  }
}

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VDD -pin_pattern {^VDDPE$}
add_global_connection -net VDD -pin_pattern {^VDDCE$}
add_global_connection -net VSS -pin_pattern {^VSS$} -ground
add_global_connection -net VSS -pin_pattern {^VSSE$}

set_voltage_domain -power VDD -ground VSS

# Define the macro grid first so it is built before the Core grid; its in-core
# metal8 stripes are then visible as candidate targets when the Core grid's pad
# direct-connections are created.
define_pdn_grid -macro -name "sram" -instances "repro_sram"
add_pdn_stripe -layer metal8 -width 1.40 -pitch 6.0 -offset 0.5
add_pdn_stripe -layer metal9 -width 1.40 -pitch 6.0 -offset 0.5
add_pdn_connect -layers {metal8 metal9}

# Core grid with grid-level pad connect.  The ring is on metal9/metal10 only,
# so the pad metal8 connection has no metal8 ring near the pads.
define_pdn_grid -name "Core" -starts_with "POWER" -connect_to_pads
add_pdn_ring -grid "Core" -layers {metal9 metal10} -widths 5.0 \
  -spacings 2.0 -core_offsets 2

add_pdn_stripe -layer metal4 -width 0.48 -pitch 56.0 -offset 2.24
add_pdn_stripe -layer metal7 -width 1.40 -pitch 40.0 -offset 2.70
add_pdn_stripe -layer metal8 -width 1.40 -pitch 200.0 -offset 100.0
add_pdn_stripe -layer metal9 -width 1.40 -pitch 40.0 -offset 2.70

add_pdn_connect -layers {metal4 metal7}
add_pdn_connect -layers {metal7 metal8}
add_pdn_connect -layers {metal8 metal9}
add_pdn_connect -layers {metal9 metal10}

pdngen

set core [$block getCoreArea]
set core_ymin [$core yMin]

# A genuine south pad connection is a narrow vertical metal8 shape rooted in the
# pad (ymin within the pad y-extent).  Assert that none extends into the core.
set pad_top 352000
set bad 0
foreach net {VDD VSS} {
  set dnet [$block findNet $net]
  foreach swire [$dnet getSWires] {
    foreach wire [$swire getWires] {
      if { [$wire isVia] } { continue }
      if { [[$wire getTechLayer] getName] != "metal8" } { continue }
      set xmin [$wire xMin]; set ymin [$wire yMin]
      set xmax [$wire xMax]; set ymax [$wire yMax]
      set dx [expr { $xmax - $xmin }]; set dy [expr { $ymax - $ymin }]
      if { $dx < 5000 && $dy > $dx && $ymin <= $pad_top && $ymax > $core_ymin } {
        incr bad
      }
    }
  }
}

puts "pad connections extending into core: $bad"
