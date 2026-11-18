# Regression test for issue #9070.
#
# CTS sink clustering must not leave any sink in a single-sink (orphan)
# cluster that bypasses the leaf-buffer level.  When that happens, the
# orphan sinks end up with one fewer buffer in their clock path than the
# clustered sinks, creating skew between them (visible on sky130hd/aes).
#
# Strategy:
#  1. Place a dense block of sinks (> 200, so sink clustering is enabled and
#     forms normal multi-sink clusters, each driven by a clkbuf_leaf_ buffer).
#  2. Place a handful of additional sinks far apart from the block and from
#     each other (farther than the cluster diameter) so each one forms its
#     own single-sink "orphan" cluster.
#
# Before the fix, single-sink clusters skipped the clkbuf_leaf_ buffer, so
# CTS reported a minimum clock-path buffer count smaller than the maximum
# (e.g. min 3 vs max 4).  After the fix, every cluster -- including
# single-sink ones -- gets a leaf buffer, so the minimum equals the maximum
# (balanced clock depth).
#
# The pass/fail signal is CTS-0012 (minimum) == CTS-0013 (maximum).

source "helpers.tcl"

proc create_orphan_test { } {
  set db [ord::get_db]
  set tech [$db getTech]
  set chip [odb::dbChip_create $db $tech]
  set block [odb::dbBlock_create $chip "orphan_test"]
  set master [$db findMaster "DFF_X1"]
  set layer [$tech findLayer "metal6"]

  $block setDefUnits [$tech getDbUnitsPerMicron]
  set rect [odb::Rect]
  $rect init 0 0 4000000 4000000
  $block setDieArea $rect

  set clk [odb::dbNet_create $block "clk"]
  set term [odb::dbBTerm_create $clk "clk"]
  $term setIoType INPUT
  set pin [odb::dbBPin_create $term]
  $pin setPlacementStatus FIRM
  odb::dbBox_create $pin $layer 0 0 2000 2000

  set ff_idx 0

  # Dense block of 16x16 = 256 sinks (above the 200-sink clustering threshold).
  set base 100000
  set step 2400
  for { set gx 0 } { $gx < 16 } { incr gx } {
    for { set gy 0 } { $gy < 16 } { incr gy } {
      set x [expr { $base + $gx * $step }]
      set y [expr { $base + $gy * $step }]
      set inst [odb::dbInst_create $block $master "ff$ff_idx"]
      $inst setOrigin $x $y
      $inst setPlacementStatus PLACED
      [$inst findITerm "CK"] connect $clk
      incr ff_idx
    }
  }

  # Isolated sinks placed far apart from each other (well beyond the cluster
  # diameter) so each forms its own single-sink (orphan) cluster.
  set far 2000000
  foreach off {0 300000 650000 1000000 1400000 1850000} {
    set xy [expr { $far + $off }]
    set inst [odb::dbInst_create $block $master "orphan$ff_idx"]
    $inst setOrigin $xy $xy
    $inst setPlacementStatus PLACED
    [$inst findITerm "CK"] connect $clk
    incr ff_idx
  }

  sta::db_network_defined
  puts "Created $ff_idx sinks total (256 clustered + 6 isolated orphans)"
}

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

create_orphan_test

create_clock -period 5 clk
set_wire_rc -clock -layer metal5

set_cts_config -sink_clustering_max_diameter 100 \
  -root_buf BUF_X4 \
  -buf_list BUF_X4

clock_tree_synthesis -sink_clustering_enable

report_cts
