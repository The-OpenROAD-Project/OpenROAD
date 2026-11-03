# Helpers shared by the scan_opt regression tests.

# Places every instance at a scattered, deterministic location. Different
# seeds give different placements.
proc scatter_insts { seed } {
  set i 0
  foreach inst [lsort [[ord::get_db_block] getInsts]] {
    set x [expr { (($i + $seed) * 7919) % 20000 }]
    set y [expr { (($i + $seed) * 104729) % 20000 }]
    $inst setLocation $x $y
    $inst setOrient R0
    $inst setPlacementStatus PLACED
    incr i
  }
}

proc scan_chain_cells { chain } {
  set cells {}
  foreach partition [$chain getScanPartitions] {
    foreach list [$partition getScanLists] {
      foreach scan_inst [$list getScanInsts] {
        lappend cells $scan_inst
      }
    }
  }
  return $cells
}

proc scan_chain_by_name { name } {
  foreach chain [[[ord::get_db_block] getDft] getScanChains] {
    if { [$chain getName] == $name } {
      return $chain
    }
  }
  error "no scan chain $name"
}

# Returns the chain's cells in odb order, each with the net on its SCD pin,
# so a change to either the odb order or the netlist shows up.
proc scan_chain_order { chain } {
  set order {}
  foreach scan_inst [scan_chain_cells $chain] {
    set inst [$scan_inst getInst]
    lappend order [list [$inst getName] [[[$inst findITerm SCD] getNet] getName]]
  }
  return $order
}

# Returns a dict of chain name -> name of its last cell.
proc scan_chain_last_cells { } {
  set lasts [dict create]
  foreach chain [[[ord::get_db_block] getDft] getScanChains] {
    set cells [scan_chain_cells $chain]
    dict set lasts [$chain getName] [[[lindex $cells end] getInst] getName]
  }
  return $lasts
}

# Checks each scan chain in odb against the netlist and prints one line per
# chain:
# - falling-edge cells come before rising-edge cells
# - the first cell's SCD is on a net with a scan-in port
# - each cell's SCD is on the net driven by the previous cell's Q
# - the last cell is the one recorded in expected_lasts, if given
proc check_scan_chains { { expected_lasts {} } } {
  set by_name [dict create]
  foreach chain [[[ord::get_db_block] getDft] getScanChains] {
    dict set by_name [$chain getName] $chain
  }
  foreach name [lsort -dictionary [dict keys $by_name]] {
    set chain [dict get $by_name $name]
    set cells [scan_chain_cells $chain]

    set edges_ok 1
    set seen_rising 0
    foreach scan_inst $cells {
      if { [$scan_inst getClockEdgeString] == "Rising" } {
        set seen_rising 1
      } elseif { $seen_rising } {
        set edges_ok 0
      }
    }

    set first [[lindex $cells 0] getInst]
    set scan_in_ok 0
    foreach bterm [[[$first findITerm SCD] getNet] getBTerms] {
      if { [string match "scan_in*" [$bterm getName]] } {
        set scan_in_ok 1
      }
    }

    set stitch_ok 1
    for { set i 1 } { $i < [llength $cells] } { incr i } {
      set prev [[lindex $cells [expr { $i - 1 }]] getInst]
      set cur [[lindex $cells $i] getInst]
      if { [[$cur findITerm SCD] getNet] != [[$prev findITerm Q] getNet] } {
        set stitch_ok 0
      }
    }

    set bits 0
    foreach scan_inst $cells {
      incr bits [$scan_inst getBits]
    }

    set last [[[lindex $cells end] getInst] getName]
    set last_ok 1
    if { [dict exists $expected_lasts $name] } {
      set last_ok [expr { [dict get $expected_lasts $name] == $last }]
    }

    puts "$name: bits $bits edges_ok $edges_ok scan_in_ok $scan_in_ok\
      stitch_ok $stitch_ok last_ok $last_ok"
  }
}
