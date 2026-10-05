# Every mode's clocks get a clock tree, not just the command mode's. "clk" is
# defined in two modes and must not be reported as overlapping itself
# (CTS-0114). "scan_clk" and "bist_clk" exist only outside the command mode,
# so their gated sub-trees are reached only if the clock nets of every mode
# are known.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib

# Clocks and periods of each mode. "func" is left as the command mode, so
# every clock but "clk" is one the command mode knows nothing about.
set mode_clocks {
  func { {clk 5} }
  test { {clk 10} {scan_clk 20} }
  bist { {bist_clk 40} }
}
set roots { clk scan_clk bist_clk }
set sinks 8

set width 200000
set height 200000

set db [ord::get_db]
set tech [$db getTech]
set chip [odb::dbChip_create $db $tech]
set block [odb::dbBlock_create $chip "multi_mode"]
$block setDefUnits [$tech getDbUnitsPerMicron]
set rect [odb::Rect]
$rect init 0 0 $width $height
$block setDieArea $rect

set layer [$tech findLayer "metal6"]
set min_width [$layer getWidth]

# Enable shared by every clock gate.
set en [odb::dbNet_create $block "en"]
set en_term [odb::dbBTerm_create $en "en"]
$en_term setIoType INPUT
set en_pin [odb::dbBPin_create $en_term]
$en_pin setPlacementStatus FIRM
odb::dbBox_create $en_pin $layer 0 0 $min_width $min_width

# One gated clock tree per root, each with its own band of sinks.
set ff [$db findMaster "DFF_X1"]
set gate [$db findMaster "AND2_X1"]
set rows [llength $roots]
set row 0
foreach root $roots {
  set root_net [odb::dbNet_create $block $root]
  $root_net setSigType CLOCK
  set term [odb::dbBTerm_create $root_net $root]
  $term setSigType CLOCK
  $term setIoType INPUT
  set pin [odb::dbBPin_create $term]
  $pin setPlacementStatus FIRM
  odb::dbBox_create $pin $layer \
    [expr ($width - $min_width) / 2] [expr $height - $min_width] \
    [expr ($width + $min_width) / 2] $height

  set cg [odb::dbInst_create $block $gate "cg_$root"]
  $cg setOrigin [expr $width / 2] [expr $height / 2]
  $cg setPlacementStatus PLACED
  [$cg findITerm "A1"] connect $root_net
  [$cg findITerm "A2"] connect $en

  set gated [odb::dbNet_create $block "${root}_gated"]
  $gated setSigType CLOCK
  [$cg findITerm "ZN"] connect $gated

  set distance [expr $width / $sinks]
  for { set i 0 } { $i < $sinks } { incr i } {
    set inst [odb::dbInst_create $block $ff "ff_${root}_$i"]
    $inst setOrigin [expr $distance / 2 + ($i * $distance)] \
      [expr $height * (2 * $row + 1) / (2 * $rows)]
    $inst setPlacementStatus PLACED
    [$inst findITerm "CK"] connect $gated
  }
  incr row
}

ord::design_created

set lib [get_full_name [lindex [get_libs *] 0]]
foreach { mode clocks } $mode_clocks {
  set_mode $mode
  foreach entry $clocks {
    create_clock -name [lindex $entry 0] -period [lindex $entry 1] \
      [get_ports [lindex $entry 0]]
  }
  define_scene scene_$mode -mode $mode -liberty $lib
}
# Leave the command mode on the one mode that does not define every clock.
set_mode func

source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal1
set_wire_rc -clock -layer metal2

# The nets the sinks of one clock root hang off. CTS reconnects them to the
# nets it creates, so a tree was built for a root iff this changes.
proc sink_nets { root } {
  set names {}
  set block [ord::get_db_block]
  foreach inst [$block getInsts] {
    if { [string match "ff_${root}_*" [$inst getName]] } {
      lappend names [[[$inst findITerm CK] getNet] getName]
    }
  }
  return [lsort -unique $names]
}

set before [dict create]
foreach root $roots {
  dict set before $root [sink_nets $root]
}

set failed [catch {
  tee -variable log {
    clock_tree_synthesis -root_buf CLKBUF_X3 -buf_list CLKBUF_X3 -wire_unit 20
  }
} message]

check "a clock defined in two modes does not stop CTS" { set failed } 0
check "the clock is not reported as overlapping itself" \
  { string match {*CTS-0114*} $log } 0

# One tree per root, with no root built twice for the modes that share it.
check "every root is found exactly once" \
  { llength [regexp -all -inline {CTS-0007} $log] } [llength $roots]
check "one clock tree per root" \
  { string match "*TritonCTS found [llength $roots] clock nets*" $log } 1

foreach { mode clocks } $mode_clocks {
  foreach entry $clocks {
    set root [lindex $entry 0]
    check "mode $mode: clock $root is built" \
      { expr { [sink_nets $root] ne [dict get $before $root] } } 1
    check "mode $mode: every sink of $root is on the clock tree" \
      { lsearch -exact [sink_nets $root] "${root}_gated" } -1
  }
}

exit_summary
