# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

sta::define_cmd_args "initialize_floorplan" {[-utilization util]\
					       [-aspect_ratio ratio]\
					       [-core_space space | {bottom top left right}]\
					       [-die_area {lx ly ux uy | x1 y1 x2 y2 ...}]\
					       [-core_area {lx ly ux uy | x1 y1 x2 y2 ...}]\
					       [-additional_sites site_names]\
					       [-site site_name]\
					       [-row_parity NONE|ODD|EVEN]\
					       [-flip_sites site_names]\
					       [-gap space]}

proc initialize_floorplan { args } {
  sta::parse_key_args "initialize_floorplan" args \
    keys {-utilization -aspect_ratio -core_space \
    -die_area -core_area \
    -site -additional_sites -row_parity -flip_sites -gap} \
    flags {}

  # Existing validation
  if { [info exists keys(-utilization)] } {
    if { [info exists keys(-core_area)] } {
      utl::error IFP 20 "-core_area cannot be used with -utilization."
    }
  }

  if { [info exists keys(-die_area)] } {
    if { [info exists keys(-core_space)] } {
      utl::error IFP 24 "-core_space cannot be used with -die_area."
    }
  }

  # Check if we're using polygon mode based on coordinate count
  set is_polygon_mode 0

  # Check die_area for polygon mode (more than 4 coordinates)
  if { [info exists keys(-die_area)] } {
    set die_coords $keys(-die_area)
    if { [llength $die_coords] > 4 } {
      set is_polygon_mode 1
    }
  }

  # Check core_area for polygon mode (more than 4 coordinates)
  if { [info exists keys(-core_area)] } {
    set core_coords $keys(-core_area)
    if { [llength $core_coords] > 4 } {
      set is_polygon_mode 1
    }
  }

  # Branch to polygon flow if polygon mode detected
  if { $is_polygon_mode } {
    ifp::make_polygon_die_helper [array get keys]
    ifp::make_polygon_rows_helper [array get keys]
    return
  } else {
    ifp::make_die_helper [array get keys]
    make_rows_helper [array get keys]
  }
}

variable make_rows_args
sta::define_cmd_args "make_rows" {
					       [-core_space space | {bottom top left right}]\
					       [-core_area {lx ly ux uy}]\
					       [-additional_sites site_names]\
					       [-site site_name]\
					       [-row_parity NONE|ODD|EVEN]\
					       [-flip_sites site_names]\
					       [-gap space]}
proc make_rows { args } {
  sta::parse_key_args "make_rows" args \
    keys {-core_space \
    -core_area -site -additional_sites -row_parity -flip_sites -gap} \
    flags {}
  make_rows_helper [array get keys]
}

proc make_rows_helper { key_array } {
  array set keys $key_array
  set site ""
  if { [info exists keys(-site)] } {
    set site [ifp::find_site $keys(-site)]
  } else {
    utl::error IFP 35 "use -site to add placement rows."
  }

  set additional_sites {}
  if { [info exists keys(-additional_sites)] } {
    foreach sitename $keys(-additional_sites) {
      lappend additional_sites [ifp::find_site $sitename]
    }
  }

  set flipped_sites {}
  if { [info exists keys(-flip_sites)] } {
    foreach sitename $keys(-flip_sites) {
      lappend flipped_sites [ifp::find_site $sitename]
    }
  }

  set row_parity "NONE"
  if { [info exists keys(-row_parity)] } {
    set row_parity $keys(-row_parity)
    if { $row_parity != "NONE" && $row_parity != "ODD" && $row_parity != "EVEN" } {
      utl::error IFP 57 "-row_parity must be NONE, ODD or EVEN"
    }
  }

  # Get gap space
  set gap [expr -(2 ** 31)]
  if { [info exists keys(-gap)] } {
    set gap [ord::microns_to_dbu $keys(-gap)]
  }

  if { [info exists keys(-core_area)] } {
    if { [info exists keys(-core_space)] } {
      utl::error IFP 60 "-core_space cannot be used with -core_area."
    }

    set core_area $keys(-core_area)
    if { [llength $core_area] != 4 } {
      utl::error IFP 59 "-core_area is a list of 4 coordinates."
    }
    lassign $core_area core_lx core_ly core_ux core_uy
    sta::check_positive_float "-core_area" $core_lx
    sta::check_positive_float "-core_area" $core_ly
    sta::check_positive_float "-core_area" $core_ux
    sta::check_positive_float "-core_area" $core_uy

    ord::ensure_linked

    # convert die/core coordinates to dbu.
    ifp::make_rows \
      [ord::microns_to_dbu $core_lx] [ord::microns_to_dbu $core_ly] \
      [ord::microns_to_dbu $core_ux] [ord::microns_to_dbu $core_uy] \
      $site \
      $additional_sites \
      $row_parity \
      $flipped_sites \
      $gap
    return
  }

  if { [info exists keys(-core_space)] } {
    set core_sp $keys(-core_space)
    if { [llength $core_sp] == 1 } {
      sta::check_positive_float "-core_space" $core_sp
      set core_sp_bottom $core_sp
      set core_sp_top $core_sp
      set core_sp_left $core_sp
      set core_sp_right $core_sp
    } elseif { [llength $core_sp] == 4 } {
      lassign $core_sp core_sp_bottom core_sp_top core_sp_left core_sp_right
      sta::check_positive_float "-core_space" $core_sp_bottom
      sta::check_positive_float "-core_space" $core_sp_top
      sta::check_positive_float "-core_space" $core_sp_left
      sta::check_positive_float "-core_space" $core_sp_right
    } else {
      utl::error IFP 58 "-core_space is either a list of 4 margins or one value for all margins."
    }

    ord::ensure_linked

    # convert spacing coordinates to dbu.
    ifp::make_rows_with_spacing \
      [ord::microns_to_dbu $core_sp_bottom] \
      [ord::microns_to_dbu $core_sp_top] \
      [ord::microns_to_dbu $core_sp_left] \
      [ord::microns_to_dbu $core_sp_right] \
      $site \
      $additional_sites \
      $row_parity \
      $flipped_sites \
      $gap
    return
  }

  utl::error IFP 62 "no -core_area or -core_space specified."
}

sta::define_cmd_args "make_tracks" {[layer]\
                                      [-x_pitch x_pitch]\
                                      [-y_pitch y_pitch]\
                                      [-x_offset x_offset]\
                                      [-y_offset y_offset]}

proc make_tracks { args } {
  sta::parse_key_args "make_tracks" args \
    keys {-x_pitch -y_pitch -x_offset -y_offset} \
    flags {}

  sta::check_argc_eq0or1 "initialize_floorplan" $args

  set tech [ord::get_db_tech]

  if { [llength $args] == 0 } {
    ifp::make_layer_tracks
  } elseif { [llength $args] == 1 } {
    set layer_name [lindex $args 0]
    set layer [$tech findLayer $layer_name]
    if { $layer == "NULL" } {
      utl::error "IFP" 10 "layer $layer_name not found."
    }
    if { [$layer getType] != "ROUTING" } {
      utl::error "IFP" 25 "layer $layer_name is not a routing layer."
    }

    if { [info exists keys(-x_pitch)] } {
      set x_pitch $keys(-x_pitch)
      set x_pitch [ifp::microns_to_mfg_grid $x_pitch]
      sta::check_positive_float "-x_pitch" $x_pitch
    } else {
      set x_pitch [$layer getPitchX]
    }

    if { [info exists keys(-x_offset)] } {
      set x_offset $keys(-x_offset)
      sta::check_positive_float "-x_offset" $x_offset
      set x_offset [ifp::microns_to_mfg_grid $x_offset]
    } else {
      set x_offset [$layer getOffsetX]
    }

    if { [info exists keys(-y_pitch)] } {
      set y_pitch $keys(-y_pitch)
      set y_pitch [ifp::microns_to_mfg_grid $y_pitch]
      sta::check_positive_float "-y_pitch" $y_pitch
    } else {
      set y_pitch [$layer getPitchY]
    }

    if { [info exists keys(-y_offset)] } {
      set y_offset $keys(-y_offset)
      sta::check_positive_float "-y_offset" $y_offset
      set y_offset [ifp::microns_to_mfg_grid $y_offset]
    } else {
      set y_offset [$layer getOffsetY]
    }
    ifp::make_layer_tracks $layer $x_offset $x_pitch $y_offset $y_pitch
  }
}

sta::define_cmd_args "insert_tiecells" {tie_pin \
                                        [-prefix prefix]
}

proc insert_tiecells { args } {
  sta::parse_key_args "insert_tiecells" args \
    keys {-prefix} \
    flags {}

  sta::check_argc_eq1 "insert_tiecells" $args

  set prefix "TIEOFF_"
  if { [info exists keys(-prefix)] } {
    set prefix $keys(-prefix)
  }

  set tie_pin_split [split $args {/}]
  set port [lindex $tie_pin_split end]
  set tie_cell [join [lrange $tie_pin_split 0 end-1] {/}]

  set master NULL
  foreach lib [[ord::get_db] getLibs] {
    set master [$lib findMaster $tie_cell]
    if { $master != "NULL" } {
      break
    }
  }
  if { $master == "NULL" } {
    utl::error "IFP" 31 "Unable to find master: $tie_cell"
  }

  set mterm [$master findMTerm $port]
  if { $master == "NULL" } {
    utl::error "IFP" 32 "Unable to find master pin: $args"
  }

  ord::ensure_linked

  ifp::insert_tiecells_cmd $mterm $prefix
}

namespace eval ifp {
proc microns_to_mfg_grid { microns } {
  set tech [ord::get_db_tech]
  if { [$tech hasManufacturingGrid] } {
    set dbu [$tech getDbUnitsPerMicron]
    set grid [$tech getManufacturingGrid]
    return [expr round(round($microns * $dbu / $grid) * $grid)]
  } else {
    return [ord::microns_to_dbu $microns]
  }
}

proc make_die_helper { key_array } {
  array set keys $key_array

  if { [info exists keys(-utilization)] } {
    set util $keys(-utilization)
    if { [info exists keys(-die_area)] } {
      utl::error IFP 14 "-die_area cannot be used with -utilization."
    }
    if { [info exists keys(-core_space)] } {
      set core_sp $keys(-core_space)
      if { [llength $core_sp] == 1 } {
        sta::check_positive_float "-core_space" $core_sp
        set core_sp_bottom $core_sp
        set core_sp_top $core_sp
        set core_sp_left $core_sp
        set core_sp_right $core_sp
      } elseif { [llength $core_sp] == 4 } {
        lassign $core_sp core_sp_bottom core_sp_top core_sp_left core_sp_right
      } else {
        utl::error IFP 13 "-core_space is either a list of 4 margins or one value for all margins."
      }
    } else {
      utl::error IFP 34 "no -core_space specified."
    }
    if { [info exists keys(-aspect_ratio)] } {
      set aspect_ratio $keys(-aspect_ratio)
      sta::check_positive_float "-aspect_ratio" $aspect_ratio
    } else {
      set aspect_ratio 1.0
    }
    ifp::make_die_util $util $aspect_ratio \
      [ord::microns_to_dbu $core_sp_bottom] \
      [ord::microns_to_dbu $core_sp_top] \
      [ord::microns_to_dbu $core_sp_left] \
      [ord::microns_to_dbu $core_sp_right]
    return
  }

  if { [info exists keys(-die_area)] } {
    if { [info exists keys(-utilization)] } {
      utl::error IFP 23 "-utilization cannot be used with -die_area."
    }
    if { [info exists keys(-aspect_ratio)] } {
      utl::error IFP 33 "-aspect_ratio cannot be used with -die_area."
    }
    set die_area $keys(-die_area)
    if { [llength $die_area] != 4 } {
      utl::error IFP 15 "-die_area is a list of 4 coordinates."
    }
    lassign $die_area die_lx die_ly die_ux die_uy
    sta::check_positive_float "-die_area" $die_lx
    sta::check_positive_float "-die_area" $die_ly
    sta::check_positive_float "-die_area" $die_ux
    sta::check_positive_float "-die_area" $die_uy

    ord::ensure_linked

    # convert die coordinates to dbu.
    ifp::make_die \
      [ord::microns_to_dbu $die_lx] [ord::microns_to_dbu $die_ly] \
      [ord::microns_to_dbu $die_ux] [ord::microns_to_dbu $die_uy]
    return
  }

  utl::error IFP 19 "no -utilization or -die_area specified."
}

proc make_polygon_die_helper { key_array } {
  array set keys $key_array

  if { ![info exists keys(-die_area)] } {
    utl::error IFP 75 "no -die_area specified for polygon floorplan."
  }
  set die_vertices $keys(-die_area)
  if { [llength $die_vertices] % 2 != 0 } {
    utl::error IFP 76 "-die_area must have an even number of coordinates (x y pairs)."
  }

  if { [llength $die_vertices] < 8 } {
    utl::error IFP 77 "-die_area must have at least 4 vertices (8 coordinates)."
  }

  # Clear any previous polygon data
  ord::ensure_linked
  # list of vertices of the polygon
  set polygon_vertices {}

  # Add die polygon points
  set point_count 0
  foreach {x y} $die_vertices {
    # Validate coordinates are numeric
    if { ![string is double -strict $x] || ![string is double -strict $y] } {
      utl::error IFP 78 "Invalid die polygon coordinate\
      at position [expr $point_count*2]: '$x $y' - must be numeric"
    }
    # Check for negative coordinates
    # if {$x < 0 || $y < 0} {
    #   utl::error IFP 79 "Die polygon coordinates must be non-negative. Found: ($x, $y)"
    # }

    lappend polygon_vertices [ord::microns_to_dbu $x]
    lappend polygon_vertices [ord::microns_to_dbu $y]

    incr point_count
  }

  utl::info IFP 5 "Added $point_count die polygon vertices to the list."
  ifp::make_polygon_die $polygon_vertices
  return
}

proc make_polygon_rows_helper { key_array } {
  array set keys $key_array

  # Validate that core_area is specified for polygon floorplan
  if { ![info exists keys(-core_area)] } {
    utl::error IFP 85 "no -core_area specified for polygonal floorplan."
  }

  # Get the site information (required for polygon rows)
  set site ""
  if { [info exists keys(-site)] } {
    set site [ifp::find_site $keys(-site)]
  } else {
    utl::error IFP 80 "use -site to add placement rows for polygon floorplan."
  }

  # Get additional sites
  set additional_sites {}
  if { [info exists keys(-additional_sites)] } {
    foreach sitename $keys(-additional_sites) {
      lappend additional_sites [ifp::find_site $sitename]
    }
  }

  # Get flipped sites
  set flipped_sites {}
  if { [info exists keys(-flip_sites)] } {
    foreach sitename $keys(-flip_sites) {
      lappend flipped_sites [ifp::find_site $sitename]
    }
  }

  # Get row parity
  set row_parity "NONE"
  if { [info exists keys(-row_parity)] } {
    set row_parity $keys(-row_parity)
    if { $row_parity != "NONE" && $row_parity != "ODD" && $row_parity != "EVEN" } {
      utl::error IFP 81 "-row_parity must be NONE, ODD or EVEN"
    }
  }

  # Get gap space
  set gap [expr -(2 ** 31)]
  if { [info exists keys(-gap)] } {
    set gap [ord::microns_to_dbu $keys(-gap)]
  }

  # Handle core polygon - this is the key difference from rectangular rows
  if { [info exists keys(-core_area)] } {
    set core_vertices $keys(-core_area)
    if { [llength $core_vertices] % 2 != 0 } {
      utl::error IFP 82 "-core_area must have an even number of coordinates (x y pairs)."
    }

    if { [llength $core_vertices] < 8 } {
      utl::error IFP 83 "-core_area must have at least 4 vertices (8 coordinates)."
    }

    # Convert micron coordinates to DBU and create polygon vertices list
    set polygon_vertices {}
    set point_count 0
    foreach {x y} $core_vertices {
      # Validate coordinates are numeric
      if { ![string is double -strict $x] || ![string is double -strict $y] } {
        utl::error IFP 84 "Invalid core polygon coordinate\
        at position [expr $point_count*2]: '$x $y' - must be numeric"
      }

      # Convert micron input to DBU
      lappend polygon_vertices [ord::microns_to_dbu $x]
      lappend polygon_vertices [ord::microns_to_dbu $y]
      incr point_count
    }

    ord::ensure_linked

    # Call the polygon rows creation function with simplified interface
    ifp::make_polygon_rows_simple \
      $polygon_vertices \
      $site \
      $additional_sites \
      $row_parity \
      $flipped_sites \
      $gap
    return
  }
}
} ;# end namespace ifp
