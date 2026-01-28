# Test if CTS is ok when location collision occurs b/w clk buffer and
# the centroid of sinks in a cluster.
source "helpers.tcl"

# Strategy:
# 1. Place FFs in a grid pattern (deterministic "random" placement)
# 2. Calculate where the cluster centroid will be for each group
# 3. Place an EXTRA FF at exactly that centroid location
# 4. When clustering runs, the cluster buffer location will collide
#    with the pre-placed FF in mapLocationToSink_
#
# Key insight:
# - All sinks are registered in mapLocationToSink_ BEFORE clustering
# - Cluster centroid = average of sink positions in that cluster
# - If a sink is ALREADY at the centroid location, collision occurs!
#
# For cluster_size=10, we need to know which 10 sinks form a cluster.
# The clustering algorithm is complex, so we use a simpler approach:
# - Place sinks in a small area so the centroid is predictable
# - Add one sink at the exact centroid of that area

proc create_collision_test { } {
  set db [ord::get_db]
  set tech [$db getTech]
  set chip [odb::dbChip_create $db $tech]
  set block [odb::dbBlock_create $chip "collision_test"]
  set master [$db findMaster "DFF_X1"]

  set layer [$tech findLayer "metal6"]

  $block setDefUnits [$tech getDbUnitsPerMicron]
  set rect [odb::Rect]
  $rect init 0 0 800000 800000
  $block setDieArea $rect

  set clk [odb::dbNet_create $block "clk"]
  set term [odb::dbBTerm_create $clk "clk"]
  $term setIoType INPUT
  set pin [odb::dbBPin_create $term]
  $pin setPlacementStatus FIRM
  odb::dbBox_create $pin $layer 400000 799000 400100 800000

  # Create groups of 11 sinks each, where we KNOW the centroid
  #
  # For 11 sinks to have centroid at (Cx, Cy):
  # - Place 10 sinks symmetrically around (Cx, Cy)
  # - The 11th sink is placed EXACTLY at (Cx, Cy)
  #
  set ff_idx 0
  set num_groups 21 ;# 21 groups * 10 sinks = 210 sinks (just above 200 threshold)

  # Cell spacing
  set cell_spacing 400 ;# ensure no overlap (DFF ~200 wide)

  # Group spacing (well beyond cluster diameter of 60um = 120000 dbu)
  set group_spacing 200000

  for { set g 0 } { $g < $num_groups } { incr g } {
    # Group center
    set group_cx [expr { 100000 + ($g % 4) * $group_spacing }]
    set group_cy [expr { 100000 + ($g / 4) * $group_spacing }]

    # For 10 sinks: offsets that sum to (0,0)
    # Use 5 pairs: each pair (a,b) and (-a,-b)
    set offsets10 {
      {-4 -4} {4 4}
      {-3 -1} {3 1}
      {-2 2}  {2 -2}
      {-1 3}  {1 -3}
      {0 -3}  {0 3}
    }
    # Check sum: x = -4+4-3+3-2+2-1+1+0+0 = 0 ✓
    #            y = -4+4-1+1+2-2+3-3-3+3 = 0 ✓
    # Average = (0,0), plus group center = group center ✓

    foreach off $offsets10 {
      set dx [lindex $off 0]
      set dy [lindex $off 1]
      set x [expr { $group_cx + $dx * $cell_spacing }]
      set y [expr { $group_cy + $dy * $cell_spacing }]

      set inst [odb::dbInst_create $block $master "ff$ff_idx"]
      $inst setOrigin $x $y
      $inst setPlacementStatus PLACED
      [$inst findITerm "CK"] connect $clk
      incr ff_idx
    }

    # Now add the COLLISION-INDUCING sink at the group center!
    # This sink's location = cluster centroid = cluster buffer location
    # When the cluster buffer tries to insert here, COLLISION!
    set inst [odb::dbInst_create $block $master "collision_ff$g"]
    $inst setOrigin $group_cx $group_cy
    $inst setPlacementStatus PLACED
    [$inst findITerm "CK"] connect $clk
    incr ff_idx
  }

  # Make STA recognize the design created by ODB APIs
  sta::db_network_defined

  puts "Created $ff_idx sinks total"
  puts "Each group has 10 sinks symmetrically placed + 1 sink at centroid"
  puts "The centroid sink causes collision with cluster buffer"
}

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

create_collision_test

create_clock -period 5 clk
set_wire_rc -clock -layer metal5

#set_debug_level CTS legalizer 4
#set_debug_level CTS clustering 4

# Cluster diameter must be larger than our group span
# Group span: 8 * 400 = 3200 dbu = 1.6 um
# Use 60um to easily capture all 11 sinks in one cluster
set_cts_config -wire_unit 1000 \
  -distance_between_buffers 100 \
  -sink_clustering_size 11 \
  -sink_clustering_max_diameter 60 \
  -num_static_layers 1 \
  -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3

clock_tree_synthesis -sink_clustering_enable

report_cts
