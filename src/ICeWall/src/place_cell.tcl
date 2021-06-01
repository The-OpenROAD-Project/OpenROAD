sta::define_cmd_args "place_cell" {-inst_name inst_name \
                                     [-cell library_cell] \
                                     -origin xy_origin \
                                     -orient (R0|R90|R180|R270|MX|MY|MXR90|MYR90) \
                                     [-status (PLACED|FIRM)]}
proc place_cell {args} {
  set db [ord::get_db]
  set block [ord::get_db_block]

  sta::parse_key_args "place_cell" args \
    keys {-cell -origin -orient -inst_name -status}

  if {[info exists keys(-status)]} {
    set placement_status $keys(-status)
    if {[lsearch {PLACED FIRM} $placement_status] == -1} {
      utl::error PAD 999 "Invalid placement status $placement_status, must be one of either PLACED or FIRM"
    }
  } else {
    set placement_status "PLACED"
  }

  if {[info exists keys(-cell)]} {
    set cell_name $keys(-cell)
    if {[set cell_master [$db findMaster $cell_name]] == "NULL"} {
      utl::error PAD 999 "Cell $cell_name not loaded into design"
    }
  }

  if {[info exists keys(-inst_name)]} {
    set inst_name [lindex $keys(-inst_name) 0]
  } else {
    utl::err PAD 999 "-inst_name is a required argument to the place_cell command"
  }

  # Verify cell orientation
  set valid_orientation {R0 R90 R180 R270 MX MY MXR90 MYR90}
  if {[info exists keys(-orient)]} {
    set orient $keys(-orient)
    if {[lsearch $valid_orientation $orient] == -1} {
      utl::error PAD 999 "Invalid orientation $orient specified, must be one of [join $valid_orientation {, }]"
    }
  } else {
    utl::error PAD 999 "No orientation specified for $inst_name"
  }

  # Verify centre/origin
  if {[info exists keys(-origin)]} {
    set origin $keys(-origin) 
    if {[llength $origin] != 2} {
      utl::error PAD 999 "Origin is $origin, but must be a list of 2 numbers"
    }
    if {[catch {set x [ord::microns_to_dbu [lindex $origin 0]]} msg]} {
      utl::error PAD 999 "Invalid value specified for x value, [lindex $origin 0], $msg"
    }
    if {[catch {set y [ord::microns_to_dbu [lindex $origin 1]]} msg]} {
      utl::error PAD 999 "Invalid value specified for y value, [lindex $origin 1], $msg"
    }
  } else {
    utl::error PAD 999 "No origin specified for $inst_name"
  }

  if {[set inst [$block findInst $inst_name]] == "NULL"} {
    if {[info exists keys(-cell)]} {
      set inst [odb::dbInst_create $block $cell_master $inst_name]
    } else {
      utl::error PAD 999 "Instance $inst_name no in the design, -cell must be specified to create a new instance"
    }
  } else {
    if {[info exists keys(-cell)]} {
      if {[[$inst getMaster] getName] != $cell_name} {
        utl::error PAD 999 "Instance $inst_name expected to be $cell_name, but is actually [[$inst getMaster] getName]"
      }
    }
  }

  if {$inst == "NULL"} {
    utl::error PAD 999 "Cannot create instance $inst_name of $cell_name"
  }

  $inst setOrigin $x $y
  $inst setOrient $orient
  $inst setPlacementStatus $placement_status
}

