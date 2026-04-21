# Test that buffers inserted by repair_design are clamped inside the core area.
# A tight core with loads manually placed outside triggers wire repeaters
# at Steiner points beyond the core boundary.
source "helpers.tcl"
source Nangate45/Nangate45.vars

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog repair_design_outside_core.v
link_design outside_core

# Tight core: (10, 10) to (90, 90) microns within a 200x200 die.
initialize_floorplan -site $site \
  -die_area {0 0 200 200} \
  -core_area {10 10 90 90}

# Place driver inside core, loads far outside core to create long wires
# whose Steiner points land outside core.
set block [ord::get_db_block]
foreach inst [$block getInsts] {
  set name [$inst getName]
  if { $name eq "drvr" } {
    $inst setLocation 50000 50000
    $inst setPlacementStatus PLACED
  } elseif { $name eq "load0" } {
    $inst setLocation 0 0
    $inst setPlacementStatus PLACED
  } elseif { $name eq "load1" } {
    $inst setLocation 195000 0
    $inst setPlacementStatus PLACED
  } elseif { $name eq "load2" } {
    $inst setLocation 0 195000
    $inst setPlacementStatus PLACED
  } elseif { $name eq "load3" } {
    $inst setLocation 195000 195000
    $inst setPlacementStatus PLACED
  } elseif { $name eq "load4" } {
    $inst setLocation 100000 0
    $inst setPlacementStatus PLACED
  } elseif { $name eq "load5" } {
    $inst setLocation 0 100000
    $inst setPlacementStatus PLACED
  } elseif { $name eq "load6" } {
    $inst setLocation 195000 100000
    $inst setPlacementStatus PLACED
  } elseif { $name eq "load7" } {
    $inst setLocation 100000 195000
    $inst setPlacementStatus PLACED
  } elseif { $name eq "load8" } {
    $inst setLocation 150000 0
    $inst setPlacementStatus PLACED
  } elseif { $name eq "load9" } {
    $inst setLocation 50000 0
    $inst setPlacementStatus PLACED
  }
}

create_clock -period 10 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# Enable debug log so we can verify per-buffer clamp messages
set_debug_level RSZ buffer_clamp 1

# Force fanout and wire length violations to trigger buffer insertion
set_max_fanout 5 [current_design]
repair_design -max_wire_length 100

# Verify: all instances must be within core area
set core [ord::get_db_core]
set core_xmin [$core xMin]
set core_ymin [$core yMin]
set core_xmax [$core xMax]
set core_ymax [$core yMax]

set outside_core 0
foreach inst [$block getInsts] {
  set bbox [$inst getBBox]
  if {
    [$bbox xMin] < $core_xmin || [$bbox yMin] < $core_ymin
    || [$bbox xMax] > $core_xmax || [$bbox yMax] > $core_ymax
  } {
    # Only check newly inserted buffers (source type TIMING), not original loads
    if { [$inst getSourceType] eq "TIMING" } {
      puts "FAIL: [$inst getName] outside core\
        ([$bbox xMin] [$bbox yMin]) ([$bbox xMax] [$bbox yMax])"
      set outside_core 1
    }
  }
}
if { !$outside_core } {
  puts "PASS: all inserted buffers are within core area."
}
