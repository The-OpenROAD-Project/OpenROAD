namespace eval ICeWall {
  variable cells {}
  variable db
  variable default_orientation {bottom R0 right R90 top R180 left R270 ll R0 lr MY ur R180 ul MX}
  variable connect_pins_by_abutment
  variable idx {fill 0}

  proc set_message {level message} {
    return "\[$level\] $message"
  }

  proc debug {message} {
    set state [info frame -1]
    set str ""
    if {[dict exists $state file]} {
      set str "$str[dict get $state file]:"
    }
    if {[dict exists $state proc]} {
      set str "$str[dict get $state proc]:"
    }
    if {[dict exists $state line]} {
      set str "$str[dict get $state line]"
    }
    puts [set_message DEBUG "$str: $message"]
  }

  proc set_footprint {footprint_data} {
    variable footprint
    
    set footprint $footprint_data
  }
  
  proc set_library {library_data} {
    variable library

    set library $library_data
  }
    
  proc get_origin {centre width height orient} {
      switch -exact $orient {
        R0    {
          set x [expr [dict get $centre x] - ($width / 2)]
          set y [expr [dict get $centre y] - ($height / 2)]
        }
        R180  {
          set x [expr [dict get $centre x] + ($width / 2)]
          set y [expr [dict get $centre y] + ($height / 2)]
        }
        MX    {
          set x [expr [dict get $centre x] - ($width / 2)]
          set y [expr [dict get $centre y] + ($height / 2)]
        }
        MY    {
          set x [expr [dict get $centre x] + ($width / 2)]
          set y [expr [dict get $centre y] - ($height / 2)]
        }
        R90   {
          set x [expr [dict get $centre x] + ($height / 2)]
          set y [expr [dict get $centre y] - ($width / 2)]
        }
        R270  {
          set x [expr [dict get $centre x] - ($height / 2)]
          set y [expr [dict get $centre y] + ($width / 2)]
        }
        MXR90 {
          set x [expr [dict get $centre x] + ($height / 2)]
          set y [expr [dict get $centre y] + ($width / 2)]
        }
        MYR90 {
          set x [expr [dict get $centre x] - ($height / 2)]
          set y [expr [dict get $centre y] - ($width / 2)]
        }
        default {critcal 5 "Illegal orientation $orient specified"}
      }

      return [list x $x y $y]
  }

  proc get_centre {centre width height orient} {
      switch -exact $orient {
        R0    {
          set x [expr [dict get $centre x] + ($width / 2)]
          set y [expr [dict get $centre y] + ($height / 2)]
        }
        R180  {
          set x [expr [dict get $centre x] - ($width / 2)]
          set y [expr [dict get $centre y] - ($height / 2)]
        }
        MX    {
          set x [expr [dict get $centre x] + ($width / 2)]
          set y [expr [dict get $centre y] - ($height / 2)]
        }
        MY    {
          set x [expr [dict get $centre x] - ($width / 2)]
          set y [expr [dict get $centre y] + ($height / 2)]
        }
        R90   {
          set x [expr [dict get $centre x] - ($height / 2)]
          set y [expr [dict get $centre y] + ($width / 2)]
        }
        R270  {
          set x [expr [dict get $centre x] + ($height / 2)]
          set y [expr [dict get $centre y] - ($width / 2)]
        }
        MXR90 {
          set x [expr [dict get $centre x] - ($height / 2)]
          set y [expr [dict get $centre y] - ($width / 2)]
        }
        MYR90 {
          set x [expr [dict get $centre x] + ($height / 2)]
          set y [expr [dict get $centre y] + ($width / 2)]
        }
        default {utl::error "PAD" 6 "Illegal orientation $orient specified"}
      }

      return [list x $x y $y]
  }

  proc get_footprint_padcells_by_side {side_name} {
    variable footprint
    if {![dict exists $footprint order $side_name]} {
      set padcells {}
      dict for {padcell data} [dict get $footprint padcell] {
        if {[dict get $data side] == $side_name} {
          lappend padcells $padcell 
        }
      }

      dict set footprint order $side_name $padcells
      # debug "Side: $side_name, Padcells: [dict get $footprint order $side_name]"
    }
    
    return [dict get $footprint order $side_name]
  }

  proc get_library_bondpad_width {} {
    variable library
    
    if {[dict exists $library types bondpad]} {
      set bondpad_cell [get_cell "bondpad" "top"]
      return [$bondpad_cell getWidth]
    }

    utl::error "PAD" 24 "Cannot find bondpad type in library"
  }
  
  proc get_library_bondpad_height {} {
    variable library
    
    if {[dict exists $library types bondpad]} {
      set bondpad_cell [get_cell "bondpad" "top"]
      return [$bondpad_cell getHeight]
    }
    
    utl::error "PAD" 26 "Cannot find bondpad type in library"
  }
  
  proc get_footprint_padcell_names {} {
    variable footprint
    
    return [dict keys [dict get $footprint padcell]]
  }

  proc get_footprint_padcell_order {} {
    variable footprint
    
    return [dict get $footprint full_order]
  }

  proc get_footprint_padcell_order_connected {} {
    variable footprint
    
    if {![dict exists $footprint connected_padcells_order]} {
      set connected_padcells {}
      foreach padcell [get_footprint_padcell_order] {
        if {[is_padcell_physical_only $padcell]} {continue} 
        if {[is_padcell_control $padcell]} {continue} 
        lappend connected_padcells $padcell
      }
      dict set footprint connected_padcells_order $connected_padcells
    }    
    return [dict get $footprint connected_padcells_order]
  }

  proc is_footprint_create_padcells {} {
    variable footprint
    
    if {[dict exists $footprint create_padcells] && [dict get $footprint create_padcells]} {
      return 1
    }
    return 0
  }
  
  proc get_padcell_inst_info {padcell} {
    variable footprint

    if {[dict exists $footprint padcell $padcell]} {
      set inst [dict get $footprint padcell $padcell]
    } elseif {[dict exists $footprint place $padcell]} {
      set inst [dict get $footprint place $padcell]
    } else {
      utl::error "PAD" 25 "No instance found for $padcell"
    }    

    return  $inst
  }
  
  proc get_scaled_location {padcell} {
    variable footprint

    if {[dict exists $footprint padcell $padcell scaled_centre]} {
      return [dict get $footprint padcell $padcell scaled_centre]
    } elseif {[dict exists $footprint padcell $padcell scaled_origin]} {
      return [dict get $footprint padcell $padcell scaled_origin]
    } elseif {[dict exists $footprint padcell $padcell cell centre]} {
      return [get_scaled_centre $padcell cell]
    } elseif {[dict exists $footprint padcell $padcell cell origin]} {
      return [get_scaled_origin $padcell cell]
    }
  }
  
  proc get_scaled_origin {padcell type} {
    variable library
    variable def_units

    set inst [get_padcell_inst_info $padcell]

    if {[dict exists $inst $type scaled_origin]} {
      set scaled_origin [dict get $inst $type scaled_origin];
    } elseif {[dict exists $inst $type origin]} {
      set scaled_origin [list \
        x [expr round([dict get $inst $type origin x] * $def_units)] \
        y [expr round([dict get $inst $type origin y] * $def_units)] \
      ]
    } else {
      if {$type == "bondpad"} {
        variable bondpad_width
        variable bondpad_height

        set width $bondpad_width
        set height $bondpad_height
        set orient [get_padcell_orient $padcell bondpad]
      } else {
        set cell_name [get_padcell_cell_name $padcell]
        set orient [get_padcell_orient $padcell]

        set cell [get_cell_master $cell_name]
        set width [$cell getWidth]
        set height [$cell getHeight]
      }

      if {[dict exists $inst $type scaled_centre]} {
        set centre [dict get $inst $type scaled_centre]
        set scaled_origin [get_origin $centre $width $height $orient]
      } elseif {[dict exists $inst $type centre]} {
        set centre [list \
          x [expr round([dict get $inst $type centre x] * $def_units)] \
          y [expr round([dict get $inst $type centre y] * $def_units)] \
        ]
        set scaled_origin [get_origin $centre $width $height $orient]
      } else {
        # debug "padcell: $padcell, data: [dict get $inst]"
      }
    }
    
    return $scaled_origin
  }
  
  proc get_scaled_centre {padcell type} {
    variable footprint
    variable library
    variable def_units

    set inst [get_padcell_inst_info $padcell]

    if {[dict exists $inst $type scaled_centre]} {
      set scaled_centre [dict get $inst $type scaled_centre];
    } elseif {[dict exists $inst $type centre]} {
      set scaled_centre [list \
        x [expr round([dict get $inst $type centre x] * $def_units)] \
        y [expr round([dict get $inst $type centre y] * $def_units)] \
      ]
    } else {
      if {$type == "bondpad"} {
        variable bondpad_width
        variable bondpad_height

        set width $bondpad_width
        set height $bondpad_height
        set orient [get_padcell_orient $padcell bondpad]
      } else {
        set side_name [get_padcell_side_name $padcell]
        if {$side_name == "none"} {
          # debug "padcell $padcell data [dict get $inst]"
          # set inst [dict get $footprint place $padcell]
          set cell_type [dict get $library types [dict get $inst type]]
          set orient [dict get $inst $type orient]
        } else {
          set cell_type [dict get $library types [get_padcell_type $padcell]]
          set orient [get_padcell_orient $padcell]
        }

        set cell_name [get_padcell_cell_name $padcell]
        set cell [get_cell_master $cell_name]
        set width [$cell getWidth]
        set height [$cell getHeight]
      }

      if {[dict exists $inst $type scaled_origin]} {
        set origin [dict get $inst $type scaled_origin]
        set scaled_centre [get_centre $origin $cell_width $cell_height $orient]
      } elseif {[dict exists $inst $type origin]} {
        set origin [list \
          x [expr round([dict get $inst $type origin x] * $def_units)] \
          y [expr round([dict get $inst $type origin y] * $def_units)] \
        ]
        set scaled_centre [get_centre $origin $cell_width $cell_height $orient]
      }
    }
    
    return $scaled_centre
  }
  
  proc get_padcell_origin {padcell {type cell}} {
    variable footprint

    if {![dict exists $footprint padcell $padcell $type scaled_origin]} {
      dict set footprint padcell $padcell $type scaled_origin [get_scaled_origin $padcell $type]
    }
    
    return [dict get $footprint padcell $padcell $type scaled_origin]
  }
  
  proc get_padcell_centre {padcell {type cell}} {
    variable footprint

    if {![dict exists $footprint padcell $padcell $type scaled_centre]} {
      dict set footprint padcell $padcell $type scaled_centre [get_scaled_centre $padcell $type]
    }
    
    return [dict get $footprint padcell $padcell $type scaled_centre]
  }

  proc get_die_area {} {
    variable footprint
    
    if {![dict exists $footprint die_area]} {
      utl::error "PAD" 31 "No die_area specified in the footprint specification"
    }
    return [dict get $footprint die_area]
  }

  proc get_core_area {} {
    variable footprint
    
    if {![dict exists $footprint core_area]} {
      if {[array names ::env CORE_AREA] != ""} {
        dict set footprint core_area $::env(CORE_AREA)
      } else {
        utl::error "PAD" 41 "A value for core_area must specified in the footprint specification, or in the environment variable CORE_AREA"
      }
    }
    return [dict get $footprint core_area]
  }

  proc get_scaled_die_area {} {
    variable footprint
    variable def_units
    
    if {![dict exists $footprint scaled die_area]} {
      set area {}
      foreach value [get_die_area] {
        lappend area [expr round($value * $def_units)]
      }
      dict set footprint scaled die_area $area
    }
    return [dict get $footprint scaled die_area]
  }

  proc get_footprint_die_size_x {} {
    variable footprint
    variable def_units
    
    return [expr round(([lindex [dict get $footprint die_area] 2] - [lindex [dict get $footprint die_area] 0]) * $def_units)]
  }

  proc get_footprint_die_size_y {} {
    variable footprint
    variable def_units
    
    return [expr round(([lindex [dict get $footprint die_area] 3] - [lindex [dict get $footprint die_area] 1]) * $def_units)]
  }

  proc get_padcell_side_name {padcell} {
    variable footprint
    
    if {[dict exists $footprint padcell $padcell side]} {
      return [dict get $footprint padcell $padcell side]
    }
    # debug "Side none for $padcell"
    return "none"
  }
  
  proc get_padcell_type {padcell} {
    variable footprint
    
    return [dict get $footprint padcell $padcell type]
  }
  
  proc get_padcell_inst_name {padcell} {
    variable footprint

    if {[is_padcell_power $padcell] || [is_padcell_ground $padcell]} {
      set inst_name "u_$padcell"
    } else {
      set info [dict get $footprint padcell $padcell]

      if {[dict exists $footprint padcell $padcell pad_inst_name]} {
        set inst_name [format [dict get $footprint padcell $padcell pad_inst_name] [get_padcell_assigned_name $padcell]]
      } elseif {[dict exists $footprint pad_inst_name]} {
        set inst_name [format [dict get $footprint pad_inst_name] [get_padcell_assigned_name $padcell]]
      } else {
        set inst_name "u_[get_padcell_assigned_name $padcell]"
      }
    }
    
    # debug "inst_name $inst_name"
    return $inst_name
  }
  
  proc set_padcell_inst {padcell inst} {
    variable footprint
    
    dict set footprint padcell $padcell inst $inst
  }
  
  proc get_padcell_inst {padcell} {
    variable footprint
    variable block
    
    if {![dict exists $footprint padcell $padcell inst]} {
      set padcell_inst_name [get_padcell_inst_name $padcell]
      # debug "Looking for padcell with inst name $padcell_inst_name"
      if {[set inst [$block findInst $padcell_inst_name]] != "NULL"} {
        set signal_name [get_padcell_signal_name $padcell]
        # debug "Pad match by name for $padcell ($signal_name)"
        set_padcell_inst $padcell [$block findInst [get_padcell_inst_name $padcell]]
      } elseif {[dict get $footprint padcell $padcell type] == "sig"} {
        set signal_name [get_padcell_signal_name $padcell]
        # debug "Try signal matching - signal: $signal_name"
        if {[is_padcell_unassigned $padcell]} {
          # debug "Pad unassigned for $padcell"
          set_padcell_inst $padcell "NULL"
        } elseif {[set net [$block findNet $signal_name]] != "NULL"} {
          # debug "Pad match by net for $padcell ($signal_name)"
          set net [$block findNet $signal_name]
          if {$net == "NULL"} {
            utl::error "PAD" 32 "Cannot find net $signal_name for $padcell in the design"
          }
          set pad_pin_name [get_padcell_pad_pin_name $padcell]
          # debug "Found net [$net getName] for $padcell"
          set found_pin 0
          foreach iTerm [$net getITerms] {
            # debug "Connection [[$iTerm getInst] getName] ([[$iTerm getMTerm] getName]) for $padcell with signal $signal_name"
            if {[[$iTerm getMTerm] getName] == $pad_pin_name} {
              # debug "Found instance [[$iTerm getInst] getName] for $padcell with signal $signal_name"
              set_padcell_inst $padcell [$iTerm getInst]
              set found_pin 1
              break
            }
          }
          if {$found_pin == 0} {
            # debug "No padcell found for signal $signal_name"
            set_padcell_inst $padcell "NULL" 
          }
        } else {
          # debug "No match for $padcell ($signal_name)"
          set_padcell_inst $padcell "NULL" 
        }
      } else {
        # debug "Pad not signal type for $padcell"
        set_padcell_inst $padcell "NULL" 
      }
    }
    
    return [dict get $footprint padcell $padcell inst]
  }

  proc get_padcell_cell_offset {padcell} {
    # debug "padcell: $padcell, cell_name: [get_padcell_cell_type $padcell]"
    return [get_library_cell_offset [get_padcell_cell_type $padcell]]
  }

  proc get_padcell_cell_type {padcell} {
    set type [get_padcell_type $padcell]

    return [get_library_cell_by_type $type]
  }
 
  proc get_library_cell_by_type {type} {
    variable library
    
    return [dict get $library types $type]
  }
  
  proc get_library_cell_offset {cell_name} {
    variable library
    variable def_units

    if {![dict exists $library cells $cell_name scaled_offset]} {
      if {[dict exists $library cells $cell_name offset]} {
        set offset [dict get $library cells $cell_name offset]
        dict set library cells $cell_name scaled_offset [list [expr round([lindex $offset 0] * $def_units)] [expr round([lindex $offset 1] * $def_units)]]
      } else {
        dict set library cells $cell_name scaled_offset {0 0}
      }
    }

    return [dict get $library cells $cell_name scaled_offset]
  }
 
  proc get_library_cell_type_offset {type} {
    variable library

    set cell_name [get_library_cell_by_type $type]
    return [get_library_cell_offset $cell_name]
  }

  proc get_library_cell_overlay {type} {
    variable library

    set cell [get_library_cell_by_type $type]
    if {[dict exists $library cells $cell overlay]} {
      return [dict get $library cells $cell overlay]
    }

    return {}
  }

  proc get_library_cell_name {type {position "none"}} {
    variable library
    # debug "cell_type $type, position $position"

    set type_name [get_library_cell_by_type $type]
    if {[dict exists $library cells $type_name]} {
      if {[dict exists $library cells $type_name cell_name]} {
        if {[dict exists $library cells $type_name cell_name $position]} {
          set cell_name [dict get $library cells $type_name cell_name $position]
        } else {
          set cell_name [dict get $library cells $type_name cell_name]
        }
      } else {
        set cell_name $type_name
      }
    } else {
      set cell_name $type_name
    }

    return $cell_name
  }

  proc get_padcell_cell_overlay {padcell} {
    variable footprint

    if {![dict exist $footprint padcell $padcell overlay]} {
      set overlay [get_library_cell_overlay [get_padcell_type $padcell]]
      dict set footprint padcell $padcell overlay $overlay
    }

    return [dict get $footprint padcell $padcell overlay]
  }

  proc get_padcell_cell_name {padcell} {
    variable footprint

    if {[dict exists $footprint padcell $padcell]} {
      set cell_type [get_padcell_type $padcell]
      set side [get_padcell_side_name $padcell]
    } elseif {[dict exists $footprint place $padcell]} {
      set cell_type [dict get $footprint place $padcell type]
      set side "none"
    }
    
    return [get_library_cell_name $cell_type $side]
  }
  
  proc get_padcell_pin_name {padcell} {
    variable footprint
    
    if {[dict exists $footprint padcell $padcell pad_pin_name]} {
      return [format [dict get $footprint padcell $padcell pad_pin_name] [get_padcell_assigned_name $padcell]]
    }

    if {[dict exists $footprint pad_pin_name]} {
      return [format [dict get $footprint pad_pin_name] [get_padcell_assigned_name $padcell]]
    }
    
    return "u_[get_padcell_assigned_name $padcell]"
  }
  
  proc get_padcell_assigned_name {padcell} {
    variable footprint 

    if {[dict exists $footprint padcell $padcell signal_name]} {
      return [dict get $footprint padcell $padcell signal_name]
    }
    return "$padcell"
  }
  
  proc get_cell_master {name} {
    variable db

    if {[set cell [$db findMaster $name]] != "NULL"} {return $cell}
    
    utl::error "PAD" 8 "Cannot find cell $name in the database"
  }

  proc get_library_cell_orientation {cell_type position} {
    variable library

    # debug "cell_type $cell_type position $position"
    if {![dict exists $library cells $cell_type orient $position]} {
      if {![dict exists $library cells $cell_type]} {
        utl::error "PAD"  96 "No cell $cell_type defined in library ([dict keys [dict get $library cells]])"
      } else {
        utl::error "PAD"  97 "No entry found in library definition for cell $cell_type on $position side"
      }
    }

    set orient [dict get $library cells $cell_type orient $position]
  }

  proc get_library_cell_parameter_default {cell_name parameter_name} {
    variable library
    
    if {![dict exists $library cells $cell_name parameter_defaults $parameter_name]} {
      return ""
    }
    return [dict get $library cells $cell_name parameter_defaults $parameter_name]
  }
    
  proc get_padcell_orient {padcell {element "cell"}} {
    variable footprint
    
    if {[dict exists $footprint padcell $padcell $element orient]} {
      set orient [dict get $footprint padcell $padcell $element orient]
    } elseif {[dict exists $footprint place $padcell]} {
      set orient [dict get $footprint place $padcell cell orient]
    } else {
      set side_name [get_padcell_side_name $padcell]
      if {$element == "cell"} {
        set orient [get_library_cell_orientation [get_library_cell_by_type [get_padcell_type $padcell]] $side_name]
      } else {
        set orient [get_library_cell_orientation [get_library_cell_by_type $element] $side_name]
      }
    }
    
    return $orient
  }
  
  proc get_cell {type {position "none"}} {
    # debug "cell_name [get_library_cell_name $type $position]"
    return [get_cell_master [get_library_cell_name $type $position]]
  }

  proc get_cells {type side} {
    variable library 

    set cell_list {}
    foreach type_name [dict get $library types $type] {
      if {[dict exists $library cells $type_name cell_name]} {
        dict set cell_list $type_name master [get_cell_master [dict get $library cells $type_name cell_name $side]]
      } else {
        dict set cell_list $type_name master [get_cell_master $type_name]
      }
    }
    return $cell_list
  }

  proc get_tracks {} {
    variable footprint 

    if {![dict exists $footprint tracks]} {
      dict set footprint tracks "tracks.info"
    }

    if {![file exists [dict get $footprint tracks]]} {
      write_track_file [dict get $footprint tracks] [dict get $footprint core_area]
    }
    
    return [dict get $footprint tracks]
  }

  proc get_library_pad_pin_name {type} {
    variable library

    if {[dict exists $library types $type]} {
      set cell_entry [dict get $library types $type]
      if {[dict exists $library cells $cell_entry]} {
        if {[dict exists $library cells $cell_entry pad_pin_name]} {
          # debug "$cell_entry [dict get $library cells $cell_entry pad_pin_name]"
          return [dict get $library cells $cell_entry pad_pin_name]
        }
      }
    }

    if {[dict exists $library pad_pin_name]} {
      return [dict get $library pad_pin_name]
    } else {
      utl::error "PAD" 33 "No value defined for pad_pin_name in the library or cell data for $type"
    }

    return "PAD"
  }

  proc get_tech_top_routing_layer {} {
    variable tech

    foreach layer [lreverse [$tech getLayers]] {
      if {[$layer getType] == "ROUTING"} {
        return [$layer getName]
      }
    }

    return [[lindex [$tech getTechLayers] end] getName]
  }
    
  proc get_library_pad_pin_layer {} {
    variable library

    if {![dict exists $library pad_pin_layer]} {
      dict set library pad_pin_layer [get_tech_top_routing_layer]
    }

    return [dict get $library pad_pin_layer]
  }
  
  proc get_footprint_pad_pin_layer {} {
    variable footprint

    if {![dict exists $footprint pin_layer]} {
      dict set footprint pin_layer [get_library_pad_pin_layer]
    }

    return [dict get $footprint pin_layer]
  }

  proc set_padcell_property {padcell key value} {
    variable footprint
    
    dict set footprint padcell $padcell $key $value
  }
  
  proc padcell_has_bondpad {padcell} {
    variable footprint
    
    return [dict exists $footprint padcell $padcell bondpad]
  }
  
  proc set_padcell_signal_name {padcell signal_name} {
    variable footprint
    
    if {[dict exists $footprint padcell $padcell]} {
      dict set footprint padcell $padcell signal_name $signal_name
      return $signal_name
    } else {
      return ""
    }
  }
  
  proc get_padcell_signal_name {padcell} {
    variable footprint

    if {![dict exists $footprint padcell $padcell]} {
      # debug "padcell: $padcell cells: [dict keys [dict get $footprint padcell]]"
      utl::error "PAD" 22 "Cannot find padcell $padcell"
    }
    
    if {![dict exists $footprint padcell $padcell signal_name]} {
      utl::error "PAD" 23 "Signal name for padcell $padcell has not been set"
    }
    
    return [dict get $footprint padcell $padcell signal_name]
  }

  proc get_library_min_bump_spacing_to_die_edge {} {
    variable library
    variable def_units
    
    if {[dict exists $library bump spacing_to_edge]} {
      return [expr round(([dict get $library bump spacing_to_edge]) * $def_units)]
    }

    utl::error "PAD" 21 "Value of bump spacing_to_edge not specified"
  }
  
  proc get_footprint_min_bump_spacing_to_die_edge {} {
    variable footprint
    variable def_units

    if {![dict exists $footprint scaled bump_spacing_to_edge]} {
      if {[dict exists $footprint bump spacing_to_edge]} {
        dict set footprint scaled bump_spacing_to_edge [expr round([dict get $footprint bump spacing_to_edge] * $def_units)]
      } else {
        dict set footprint scaled bump_spacing_to_edge [get_library_min_bump_spacing_to_die_edge]
      }
    }
    
    return [dict get $footprint scaled bump_spacing_to_edge]
  }

  proc set_padcell_row_col {padcell row col} {
    variable footprint

    dict set footprint padcell $padcell bump row $row 
    dict set footprint padcell $padcell bump col $col
    
    dict set footprint bump $row $col padcell $padcell
  }
  
  proc set_padcell_rdl_trace {padcell path} {
    variable footprint
    
    dict set footprint padcell $padcell rdl_trace $path
  }
  
  proc get_padcell_rdl_trace {padcell} {
    variable footprint
    
    if {![dict exists $footprint padcell $padcell rdl_trace]} {
      [dict set $footprint padcell $padcell rdl_trace ""
    }
    
    return [dict get $footprint padcell $padcell rdl_trace]
  }
  
  proc get_bump_name_at_row_col {row col} {
    variable footprint

    if {![dict exists $footprint bump $row $col name]} {
      set signal_name [get_bump_signal_name $row $col]
      dict set footprint bump $row $col name "bump_${row}_${col}_${signal_name}"
    }
    
    return [dict get $footprint bump $row $col name]
  }
  
  proc get_padcell_bump_name {padcell} {
    variable footprint

    if {![dict exists $footprint padcell $padcell bump name]} {
      set row [dict get $footprint padcell $padcell bump row]
      set col [dict get $footprint padcell $padcell bump col]

      set signal_name [get_bump_signal_name $row $col]

      set bump_name "bump_${row}_${col}_${signal_name}"
      dict set footprint padcell $padcell bump name $bump_name
    }
    
    return [dict get $footprint padcell $padcell bump name]
  }
  
  proc get_padcell_at_row_col {row col} {
    variable footprint

    if {![dict exists $footprint bump $row $col padcell]} {
      foreach padcell [dict keys [dict get $footprint padcell]] {
        if {[dict exists $footprint padcell $padcell bump row] && [dict exists $footprint padcell $padcell bump row]} {
          dict set footprint bump [dict get $footprint padcell $padcell bump row] [dict get $footprint padcell $padcell bump col] padcell $padcell
        }
      }
    }
        
    if {![dict exists $footprint bump $row $col padcell]} {
      dict set footprint bump $row $col padcell ""
    }
    # debug "($row, $col) [dict get $footprint bump $row $col padcell]"
    
    return [dict get $footprint bump $row $col padcell]
  }
  
  proc get_padcell_bump_origin {padcell} {
    variable footprint

    set row [dict get $footprint padcell $padcell row]
    set col [dict get $footprint padcell $padcell col]

    return [get_bump_origin $row $col]
  }
  
  proc get_bump_centre {row col} {
    variable actual_tile_offset_x
    variable actual_tile_offset_y
    variable num_bumps_y

    set pitch [get_footprint_bump_pitch]

    return [list \
      x [expr $actual_tile_offset_x + $pitch / 2 + ($col - 1) * $pitch] \
      y [expr $actual_tile_offset_y + $pitch / 2 + ($num_bumps_y - $row) * $pitch] \
    ]
  }
  
  proc get_bump_origin {row col} {
    variable actual_tile_offset_x
    variable actual_tile_offset_y
    variable num_bumps_y

    set centre [get_bump_centre $row $col]
    set bump_width [get_library_bump_width]
    # debug "bump_width : [expr $bump_width / 2000.0]"

    return [list \
      x [expr [dict get $centre x] - $bump_width / 2] \
      y [expr [dict get $centre y] - $bump_width / 2] \
    ]
  }
  
  proc get_bump_signal_name {row col} {
    variable footprint

    set padcell [get_padcell_at_row_col $row $col]

    if {$padcell != ""} {
      return [get_padcell_signal_name $padcell]
    }
    
    if {$row % 2 == 1} {
      return "VDD"
    }
    
    return "VSS"
  }
  
  proc get_padcell_bondpad_origin {padcell} {
    return [get_padcell_origin $padcell bondpad]
  }
  
  proc get_padcell_bondpad_centre {padcell} {
    return [get_padcell_centre $padcell bondpad]
  }

  proc get_padcell_parameter {padcell parameter_name} {
    variable footprint
    
    if {![dict exists $footprint padcell $padcell parameters $parameter_name]} {
      set value [get_library_cell_parameter_default [get_padcell_cell_name $padcell] $parameter_name]
      dict set footprint padcell $padcell parameters $parameter_name $value
    }
    
    return [dict get $footprint padcell $padcell parameters $parameter_name]
  }
  
  proc is_footprint_wirebond {} {
    variable footprint

    if {[dict exists $footprint Type]} {
      return [expr {[dict get $footprint Type] == "Wirebond"}]
    }
    
    if {[dict exists $footprint type]} {
      return [expr {[dict get $footprint type] == "wirebond"}]
    }

    return 0
  }

  proc is_footprint_flipchip {} {
    variable footprint
    
    if {[dict exists $footprint Type]} {
      return [expr {[dict get $footprint Type] == "Flipchip"}]
    }
    
    if {[dict exists $footprint type]} {
      return [expr {[dict get $footprint type] == "flipchip"}]
    }

    return 0
  }

  proc init_library_bondpad {} {
    variable library
    variable bondpad_width
    variable bondpad_height

    if {[dict exists $library types bondpad]} {
      set bondpad_cell [get_cell "bondpad" "top"]
      set bondpad_width [$bondpad_cell getWidth]
      set bondpad_height [$bondpad_cell getHeight]
    }
  }

  proc set_footprint_padcells_order {side_name order} {
    variable footprint 
    
    dict set footprint order $side_name $order

    set full_order {}
    foreach side_name {bottom right top left} { 
      if {[dict exists $footprint order $side_name]} {
        set full_order [concat $full_order [dict get $footprint order $side_name]]
      }
    }
    dict set footprint full_order $full_order
  }
  
  proc get_footprint_padcells_order {} {
    variable footprint

    if {![dict exists $footprint full_order]} {
      foreach side_name {bottom right top left} { 
        set unordered_keys {}

        foreach padcell [get_footprint_padcells_by_side $side_name] {
          variable footprint
          # debug "padcell: $padcell, data: [dict get $footprint padcell $padcell]"
          set origin [get_padcell_origin $padcell]

          if {$side_name == "top" || $side_name == "bottom"} {
            lappend unordered_keys [list $padcell [dict get $origin x]]
          } else {
            lappend unordered_keys [list $padcell [dict get $origin y]]
          }
        }

        set ordered_keys {}
        if {$side_name == "bottom" || $side_name == "right"} {
          set ordered_keys [lsort -integer -index 1 $unordered_keys]
        } else {
          set ordered_keys [lsort -integer -decreasing -index 1 $unordered_keys]
        }

        set order {}
        foreach element $ordered_keys {
          lappend order [lindex $element 0]
        }
        set_footprint_padcells_order $side_name $order
      }
    }

    return [dict get $footprint full_order]
  }

  proc place_padcell_overlay {padcell} {
    variable block

    set overlay [get_padcell_cell_overlay $padcell]
    set inst [get_padcell_inst $padcell]
    if {$inst != "NULL" && [llength $overlay] > 0} {
      set overlay_inst [odb::dbInst_create $block [get_cell_master $overlay] ${padcell}_overlay]

      $overlay_inst setOrient [$inst getOrient]
      $overlay_inst setOrigin {*}[$inst getOrigin]
      $overlay_inst setPlacementStatus "FIRM"
      # debug "padcell $padcell [$inst getOrigin] [$inst getOrient]"
      # debug "overlay [$overlay_inst getName] [$overlay_inst getOrigin] [$overlay_inst getOrient]"
    }
  }
 
  proc place_padcells {} {
    variable block
    
    foreach padcell [get_footprint_padcell_names] {
      # Ensure instance exists in the design
      set name [get_padcell_inst_name $padcell]
      set cell [get_padcell_cell_name $padcell]

      # debug "name: $name, cell $cell [get_padcell_assigned_name $padcell]"
      set inst [get_padcell_inst $padcell]
      if {$inst == "NULL"} {
        # debug "No inst for $padcell"
        if {[is_padcell_physical_only $padcell]} { 
          # debug "Create physical_only cell $cell with name $name for padcell $padcell"
          set_padcell_inst $padcell [odb::dbInst_create $block [get_cell_master $cell] $name]
        } elseif {[is_padcell_control $padcell]} {
          # debug "Create control cell $cell with name $name for padcell $padcell"
          set_padcell_inst $padcell [odb::dbInst_create $block [get_cell_master $cell] $name]
          # debug [get_padcell_inst $padcell]
        } elseif {[is_footprint_create_padcells]} {
          # debug "Create cell $cell with name $name for padcell $padcell"
          set_padcell_inst $padcell [odb::dbInst_create $block [get_cell_master $cell] $name]
        } elseif {[is_padcell_power $padcell] || [is_padcell_ground $padcell]} {
          # debug "Create power/ground $cell with name $name for padcell $padcell"
          set_padcell_inst $padcell [odb::dbInst_create $block [get_cell_master $cell] $name]
        } elseif {[is_padcell_unassigned $padcell]} {
          # debug "Unassigned pad added named $name"
          set_padcell_inst $padcell "NULL"
          # set_padcell_inst $padcell [odb::dbInst_create $block [get_cell_master $cell] $name]
        } else {
          utl::warn "PAD" 11 "Expected instance $name for padcell $padcell not found"
          continue
        }
      }
    }
  }

  variable bump_to_edge_pattern {
    1 {0}
    2 {1 0}
    3 {1 0 2}
    4 {3 1 0 2}
    5 {3 1 0 2 4}
  }
  
  proc assign_padcells_to_bumps {} {
    variable num_bumps_x
    variable num_bumps_y
    variable bump_to_edge_pattern

    set corner_size [get_footprint_corner_size]
    set signal_depth $corner_size
    
    set num_signals_in_corners 0
    for {set i [expr $corner_size - 1]} {$i > 0} {incr i -1} {
      set num_signals_in_corners [expr $num_signals_in_corners + $i]
    }

    set num_signals_top_bottom [expr ($num_bumps_x - 2 * $corner_size + 1) * $signal_depth + $num_signals_in_corners * 2]
    set num_signals_left_right [expr ($num_bumps_y - 2 * $corner_size + 1) * $signal_depth + $num_signals_in_corners * 2]

    # debug "Bumps ($num_bumps_x $num_bumps_y)"
    # debug "Top/Bottom: $num_signals_top_bottom"
    # debug "Left/Right: $num_signals_left_right"
    # debug "Corner_size: $corner_size"
    
    set padcells [get_footprint_padcell_order_connected]
    if {[set required [llength $padcells]] > [expr 2 * ($num_signals_top_bottom + $num_signals_left_right)]} {
      utl::error "PAD" 2 "Not enough bumps: available [expr 2 * ($num_signals_top_bottom + $num_signals_left_right)] required $required"
    }
    # debug "available [expr 2 * ($num_signals_top_bottom + $num_signals_left_right)] required $required"
    
    # Bottom side
    set idx 0
    set row_idx 0
    set col_idx 1
    foreach padcell [lrange $padcells 0 [expr $num_signals_top_bottom - 1]] {
      if {$idx < $num_signals_in_corners} {
        # Signal is in the corner
        set row [expr $num_bumps_y - [lindex [dict get $bump_to_edge_pattern $col_idx] $row_idx]]
        set col $col_idx
        # debug "assign $padcell ($row $col) - corner bottom left"
        set_padcell_row_col $padcell $row $col
        incr row_idx
        if {$row_idx >= [llength [dict get $bump_to_edge_pattern $col_idx]]} {
          set row_idx 0
          incr col_idx
        }
      } elseif {$idx < $num_signals_top_bottom - $num_signals_in_corners} {
        # Signal is in the main body of the side signals
        set row [expr $num_bumps_y - [lindex [dict get $bump_to_edge_pattern $corner_size] [expr $row_idx % $corner_size]]]
        set col $col_idx
        # debug "assign $padcell ($row $col) - bottom"
        set_padcell_row_col $padcell $row $col
        incr row_idx
        if {[expr ($idx - $num_signals_in_corners + 1) % $corner_size] == 0} {
          set row_idx 0
          incr col_idx
        }
      } else {
        # Signal is in the far corner
        set row [expr $num_bumps_y - [lindex [dict get $bump_to_edge_pattern [expr $num_bumps_x + 1 - $col_idx]] $row_idx]]
        set col $col_idx
        # debug "assign $padcell ($row $col) - corner bottom right"
        set_padcell_row_col $padcell $row $col
        incr row_idx
        if {$row_idx == [expr $num_bumps_x + 1 - $col_idx]} {
          set row_idx 0
          incr col_idx
        }
      }
      incr idx
    }

    # Right side
    set idx 0
    set row_idx 1
    set col_idx 0
    foreach padcell [lrange $padcells $num_signals_top_bottom [expr $num_signals_top_bottom + $num_signals_left_right -1]] {
      if {$idx < $num_signals_in_corners} {
        # Signal is in the corner
        set row [expr $num_bumps_y - $row_idx]
        set col [expr $num_bumps_x - [lindex [dict get $bump_to_edge_pattern $row_idx] $col_idx]]
        # debug "assign $padcell ($row $col) - corner right bottom"
        set_padcell_row_col $padcell $row $col
        incr col_idx
        if {$row_idx == $col_idx} {
          incr row_idx
          set col_idx 0
        }
      } elseif {$idx < $num_signals_left_right - $num_signals_in_corners} {
        # Signal is in the main body of the side signals
        set row [expr $num_bumps_y - $row_idx]
        set col [expr $num_bumps_x - [lindex [dict get $bump_to_edge_pattern $corner_size] $col_idx]]
        # debug "assign $padcell ($row $col) - right"
        set_padcell_row_col $padcell $row $col
        incr col_idx
        if {[expr ($idx - $num_signals_in_corners + 1) % $corner_size] == 0} {
          incr row_idx
          set col_idx 0
        }
      } else {
        # Signal is in the far corner
        set row [expr $num_bumps_y - $row_idx]
        set col [expr $num_bumps_x - [lindex [dict get $bump_to_edge_pattern [expr $num_bumps_y - 1 - $row_idx]] $col_idx]]
        # debug "assign $padcell ($row $col) - corner right top"
        set_padcell_row_col $padcell $row $col
        incr col_idx
        if {$row_idx == [expr $num_bumps_x - 1 - $col_idx]} {
          incr row_idx
          set col_idx 0
        }
      }
      incr idx
    }

    # Top side
    set idx 0
    set row_idx 0
    set col_idx 1
    foreach padcell [lrange $padcells [expr $num_signals_top_bottom + $num_signals_left_right] [expr $num_signals_top_bottom * 2 + $num_signals_left_right - 1]] {
      if {$idx < $num_signals_in_corners} {
        # Signal is in the corner
        set row [expr [lindex [dict get $bump_to_edge_pattern $col_idx] $row_idx] + 1]
        set col [expr $num_bumps_x + 1 - $col_idx]
        # debug "assign $padcell ($row $col) - corner top right"
        set_padcell_row_col $padcell $row $col
        incr row_idx
        if {$row_idx == $col_idx} {
          set row_idx 0
          incr col_idx
        }
      } elseif {$idx < $num_signals_top_bottom - $num_signals_in_corners} {
        # Signal is in the main body of the side signals
        set row [expr [lindex [dict get $bump_to_edge_pattern $corner_size] [expr ($row_idx - $num_signals_in_corners) % $corner_size]] + 1]
        set col [expr $num_bumps_x + 1 - $col_idx]
        # debug "assign $padcell ($row $col) - top"
        set_padcell_row_col $padcell $row $col
        incr row_idx
        if {[expr ($idx - $num_signals_in_corners + 1) % $corner_size] == 0} {
          set row_idx 0
          incr col_idx
        }
      } else {
        # Signal is in the far corner
        set row [expr [lindex [dict get $bump_to_edge_pattern [expr $num_bumps_x + 1 - $col_idx]] $row_idx] + 1]
        set col [expr $num_bumps_x + 1 - $col_idx]
        # debug "assign $padcell ($row $col) - corner top left"
        set_padcell_row_col $padcell $row $col
        incr row_idx
        if {$row_idx == [expr $num_bumps_x + 1 - $col_idx]} {
          set row_idx 0
          incr col_idx
        }
      }
      incr idx
    }
   # Left side
    set idx 0
    set row_idx 1
    set col_idx 0
    foreach padcell [lrange $padcells [expr $num_signals_top_bottom * 2 + $num_signals_left_right] end] {
      if {$idx < $num_signals_in_corners} {
        # Signal is in the corner
        set row [expr $row_idx + 1]
        set col [expr [lindex [dict get $bump_to_edge_pattern $row_idx] $col_idx] + 1]
        # debug "assign $padcell ($row $col) - corner left top"
        set_padcell_row_col $padcell $row $col
        incr col_idx
        if {$row_idx == $col_idx} {
          set col_idx 0
          incr row_idx
        }
      } elseif {$idx < $num_signals_left_right - $num_signals_in_corners} {
        # Signal is in the main body of the side signals
        set row [expr $row_idx + 1]
        set col [expr [lindex [dict get $bump_to_edge_pattern $corner_size] [expr ($col_idx - $num_signals_in_corners) % $corner_size]] + 1]
        # debug "assign $padcell ($row $col) - left"
        set_padcell_row_col $padcell $row $col
        incr col_idx
        if {[expr ($idx - $num_signals_in_corners + 1) % $corner_size] == 0} {
          set col_idx 0
          incr row_idx
        }
      } else {
        # Signal is in the far corner
        set row [expr $num_bumps_y - 1 - $row_idx]
        set col [lindex [dict get $bump_to_edge_pattern $row] $col_idx]
        # debug "assign $padcell ($row $col) - corner left bottom"
        set_padcell_row_col $padcell $row $col
        incr col_idx
        if {$row_idx == [expr $num_bumps_y - 1 - $col_idx]} {
          set col_idx 0
          incr row_idx
        }
      }
      incr idx
    }
  }
  
  proc write_track_file {track_info_file core_area} {
    variable db
    variable tech 
    variable def_units
    set ch [open $track_info_file "w"]

    set tech [$db getTech]
    foreach layer [$tech getLayers] {
      if {[$layer getType] == "ROUTING"} {
        set x_offset [expr round([lindex $core_area 0] * $def_units) % [$layer getPitchX]]
        set y_offset [expr round([lindex $core_area 1] * $def_units) % [$layer getPitchY]]

        puts $ch [format "%s Y %.3f %.3f" [$layer getName] [expr 1.0 * $x_offset / $def_units] [expr 1.0 * [$layer getPitchX] / $def_units]]
        puts $ch [format "%s X %.3f %.3f" [$layer getName] [expr 1.0 * $y_offset / $def_units] [expr 1.0 * [$layer getPitchY] / $def_units]]
      }
    }
    
    close $ch
  }
  
  proc read_signal_assignments {signal_assignment_file} {
    if {![file exists $signal_assignment_file]} {
      utl::error "PAD" 7 "File $signal_assignment_file not found"
    }
    set errors {}
    set ch [open $signal_assignment_file]
    while {![eof $ch]} {
      set line [gets $ch]
      set line [regsub {\#.} $line {}]
      if {[llength $line] == 0} {continue}
      
      set pad_name [lindex $line 0]
      set signal_name [lindex $line 1]
      
      if {[set_padcell_signal_name $pad_name $signal_name] == ""} {
        lappend errors "Pad id $pad_name not found in footprint"
      } else {
        dict for {key value} [lrange $line 2 end] {
          set padcell_property $pad_name $key $value
        }
      }
    }
    
    if {[llength $errors] > 0} {
      set str "\n"
      foreach msg $errors {
         set str "$str\n  $msg"
      }
      utl::error "PAD" 1 "$str\nIncorrect signal assignments ([llength $errors]) found"
    }
    
    close $ch
  }
  
  proc assign_signals {} {
    variable signal_assignment_file

    if {$signal_assignment_file != ""} {
      read_signal_assignments $signal_assignment_file
    }
  }

  proc get_library_cells {} {
    variable library
    
    foreach type [get_library_types] {
      set cell_name [dict get $library types $type]
      if {[dict exists $library cells $cell_name cell_name]} {
        dict for {key actual_cell_name} [dict exists $library cells $cell_name cell_name] {
          lappend cells $actual_cell_name
        }
      } else {
        lappend cells $cell_name
      }
    }
  }

  proc get_library_cells_in_design {} {
    variable block
    
    set library_cells [get_library_cells]
    set existing_library_components {}
    
    foreach inst [$block getInsts] {
      set cell_name [[$inst getMaster] getName]
      
      if {[lsearch $library_cells $cell_name] > -1} {
        lappend existing_io_components $inst
      }
    }
    
    return $existing_io_components
  }

  proc get_design_io {} {
    variable block
    variable footprint
    
    if {![dict exists $footprint block_io]} {
      foreach bterm [$block getBTerms] {
        lappend io_names [$bterm getName]
      }

      dict set footprint block_io $io_names
    }

    return [dict get $footprint block_io]
  }
  
  proc assign_signal_pads_to_ios {} {
    variable $block

    foreach io_signal [get_design_io] {
      
    }
  }
  
  proc load_footprint {footprint_file} {
    variable footprint
    variable db
    variable tech
    variable block
    variable def_units
    variable chip_width 
    variable chip_height

    source $footprint_file

    set db [::ord::get_db]
    set tech [$db getTech]
    set block [[$db getChip] getBlock]

    set def_units [$tech getDbUnitsPerMicron]

    set chip_width  [get_footprint_die_size_x]
    set chip_height [get_footprint_die_size_y]
    
    set_offsets     

    # Allow us to lookup which side a padcell is placed on.
    foreach side_name {bottom right top left} { 
      if {[dict exists $footprint padcells $side_name]} {
        # debug "$side_name [dict get $footprint padcells $side_name]"
        foreach padcell [dict keys [dict get $footprint padcells $side_name]] {
          dict set footprint padcell $padcell [dict get $footprint padcells $side_name $padcell]
          if {![dict exists $footprint padcell $padcell side]} {
            dict set footprint padcell $padcell side $side_name
          }
        }
      }
    }
        
    # Lookup side assignment
    foreach padcell [dict keys [dict get $footprint padcell]] {
      # debug [dict get $footprint padcell $padcell]
      if {![dict exists $footprint padcell $padcell side]} {
        set location [get_scaled_location $padcell]
        switch [pdngen::get_quadrant [get_scaled_die_area] [dict get $location x] [dict get $location y]] {
          "b" {set side_name bottom}
          "r" {set side_name right}
          "t" {set side_name top}
          "l" {set side_name left}
        }
        # debug "$padcell, $location, $side_name"
        dict set footprint padcell $padcell side $side_name
      }
    }
  }

  proc get_padcell_pad_pin_name {padcell} {
    variable block
    variable tech
    
    return [get_library_pad_pin_name [get_padcell_type $padcell]]
  }
  
  proc get_library_pad_pin_shape {padcell} {
    variable block
    variable tech
    # debug "$padcell"
    
    set inst [get_padcell_inst $padcell]
    # debug "[$inst getName]"
    # debug "[[$inst getMaster] getName]"
    # debug "[get_padcell_type $padcell]"
    
    set mterm [[$inst getMaster] findMTerm [get_padcell_pad_pin_name $padcell]]
    set mpin  [lindex [$mterm getMPins] 0]

    foreach geometry [$mpin getGeometry] {
      if {[[$geometry getTechLayer] getName] == [get_footprint_pad_pin_layer]} {
        set pin_box [list [$geometry xMin] [$geometry yMin] [$geometry xMax] [$geometry yMax]]
        return $pin_box
      }
    } 
  }

  proc get_padcell_pad_pin_shape {padcell} {
    set inst [get_padcell_inst $padcell]
    set pin_box [pdngen::transform_box {*}[get_library_pad_pin_shape $padcell] [$inst getOrigin] [$inst getOrient]]
    return $pin_box
  }

  proc get_box_centre {box} {
    return [list [expr ([lindex $box 2] + [lindex $box 0]) / 2] [expr ([lindex $box 3] + [lindex $box 1]) / 2]]
  }

  proc add_physical_pin {padcell inst} {
    variable block
    variable tech

    set term [$block findBTerm [get_padcell_pin_name $padcell]]
    if {$term != "NULL"} {
      set net [$term getNet]
      foreach iterm [$net getITerms] {
        $iterm setSpecial
      }
      $term setSpecial
      $net setSpecial

      set pin [odb::dbBPin_create $term]
      set layer [$tech findLayer [get_footprint_pad_pin_layer]]

      if {[set mterm [[$inst getMaster] findMTerm [get_library_pad_pin_name [get_padcell_type $padcell]]]] == "NULL"} {
        utl::warn "PAD" 20 "Cannot find pin [get_library_pad_pin_name [get_padcell_type $padcell]] on cell [[$inst getMaster] getName]"
        return 0
      } else {
        set mpin [lindex [$mterm getMPins] 0]
        foreach geometry [$mpin getGeometry] {
          if {[[$geometry getTechLayer] getName] == [get_footprint_pad_pin_layer]} {
            set pin_box [pdngen::transform_box [$geometry xMin] [$geometry yMin] [$geometry xMax] [$geometry yMax] [$inst getOrigin] [$inst getOrient]]
            odb::dbBox_create $pin $layer {*}$pin_box
            $pin setPlacementStatus "FIRM"

            return 1
          }
        } 
        if {[[$geometry getTechLayer] getName] != [get_footprint_pad_pin_layer]} {
          utl::warn "PAD" 19 "Cannot find shape on layer [get_footprint_pad_pin_layer] for [$inst getName]:[[$inst getMaster] getName]:[$mterm getName]"
          return 0
        }
      }
    }
    if {[get_padcell_type $padcell] == "sig"} {
      utl::warn "PAD" 4 "Cannot find a terminal [get_padcell_pin_name $padcell] for ${padcell}" 
    }
  }

  proc connect_to_bondpad_or_bump {inst centre padcell} {
    variable block
    variable tech
    
    set assigned_name [get_padcell_assigned_name $padcell]
    if {[is_power_net $assigned_name]} {
      set type "POWER"
    } elseif {[is_ground_net $assigned_name]} {
      set type "GROUND"
    } else {
      set type "SIGNAL"
    }

    set term [$block findBTerm [get_padcell_pin_name $padcell]]

    if {$term != "NULL"} {
      set net [$term getNet]
      foreach iterm [$net getITerms] {
        $iterm setSpecial
      }
    } else {
      if {$type != "SIGNAL"} {
        set net [$block findNet $assigned_name] 
        if {$net == "NULL"} {
          if {$type == "POWER" || $type == "GROUND"} {
            set net [odb::dbNet_create $block $assigned_name]
          }
          if {$net == "NULL"} {
            continue
          }
        }
        if {[set term [$block findBTerm $assigned_name]] == "NULL"} {
          set term [odb::dbBTerm_create $net $assigned_name]
          $term setSigType $type
        }
      } elseif {[is_padcell_unassigned $padcell]} {
        set idx 0
        while {[$block findNet "_UNASSIGNED_$idx"] != "NULL"} {
          incr idx
        }
        set net [odb::dbNet_create $block "_UNASSIGNED_$idx"]
        set term [odb::dbBTerm_create $net "_UNASSIGNED_$idx"]
      } else {
        utl::warn "PAD" 5 "Cannot find a terminal [get_padcell_pin_name $padcell] for $padcell to associate with bondpad [$inst getName]"
        return
      }
    }

    $net setSpecial
    $net setSigType $type

    set pin [odb::dbBPin_create $term]
    set layer [$tech findLayer [get_footprint_pad_pin_layer]]

    set x [dict get $centre x]
    set y [dict get $centre y]

    odb::dbBox_create $pin $layer [expr $x - [$layer getWidth] / 2] [expr $y - [$layer getWidth] / 2] [expr $x + [$layer getWidth] / 2] [expr $y + [$layer getWidth] / 2]
    $pin setPlacementStatus "FIRM"
  }
  
  proc place_bondpads {} {
    variable block
    variable tech
    
    foreach side_name {bottom right top left} {
      foreach padcell [get_footprint_padcells_by_side $side_name] {
        set signal_name [get_padcell_inst_name $padcell]
        set type [get_padcell_type $padcell]
        
        if {[padcell_has_bondpad $padcell]} {
          set cell [get_cell bondpad $side_name]
          # Padcells have separate bondpads that need to be added to the design
          set origin [get_padcell_bondpad_origin $padcell]
          set inst [odb::dbInst_create $block $cell "bp_${signal_name}"]

          $inst setOrigin [dict get $origin x] [dict get $origin y]
          $inst setOrient [get_padcell_orient $padcell bondpad]
          $inst setPlacementStatus "FIRM"

          set centre [get_padcell_centre $padcell bondpad]
          connect_to_bondpad_or_bump $inst $centre $padcell
        } else {
          if {[set inst [get_padcell_inst $padcell]] == "NULL"} {
            utl::warn "PAD" 99 "No padcell instance found for $padcell"
            continue
          }
          add_physical_pin $padcell [get_padcell_inst $padcell]
        }
      }
    }
  }

  proc init_rdl {} {
    variable num_bumps_x
    variable num_bumps_y
    variable actual_tile_offset_x
    variable actual_tile_offset_y

    set die_size_x [get_footprint_die_size_x]
    set die_size_y [get_footprint_die_size_y]
    set min_bump_spacing_to_die_edge [get_footprint_min_bump_spacing_to_die_edge]
    set pitch [get_footprint_bump_pitch]
    set bump_width [get_footprint_bump_width]
    # debug "$pitch $bump_width"

    set tile_spacing_to_die_edge [expr $min_bump_spacing_to_die_edge - ($pitch - $bump_width) / 2]
    set available_bump_space_x [expr $die_size_x - 2 * $tile_spacing_to_die_edge]
    set available_bump_space_y [expr $die_size_y - 2 * $tile_spacing_to_die_edge]
    set num_bumps_x [expr int(1.0 * $available_bump_space_x / $pitch)]
    set num_bumps_y [expr int(1.0 * $available_bump_space_y / $pitch)]
    set actual_tile_offset_x [expr ($die_size_x - $num_bumps_x * $pitch) / 2]
    set actual_tile_offset_y [expr ($die_size_y - $num_bumps_y * $pitch) / 2]
    
    # debug "tile_spacing_to_die_edge $tile_spacing_to_die_edge"

    # debug "available_bump_space_x $available_bump_space_x"
    # debug "available_bump_space_y $available_bump_space_y"

    # debug "num_bumps_x $num_bumps_x"
    # debug "num_bumps_y $num_bumps_y"
  }

  proc get_bump_pitch_table {} {
    variable library
    variable def_units
    
    if {![dict exists $library scaled lookup_by_pitch]} {
      if {[dict exists $library lookup_by_pitch]} {
        dict for {key value} [dict get $library lookup_by_pitch] {
          set scaled_key [expr round($key * $def_units)]
          dict set library scaled lookup_by_pitch $scaled_key $value
        }
      } else {
        utl::error "PAD" 34 "No bump pitch table defined in the library"
      }
    }
    return [dict get $library scaled lookup_by_pitch]
  }
  
  proc lookup_by_bump_pitch {data} {
    variable library 
    
    set pitch [get_footprint_bump_pitch]

    set pitch_list [lreverse [lsort -integer [dict keys $data]]]
    if {$pitch >= [lindex $pitch_list 0]} {
      return [dict get $data [lindex $pitch_list 0]]
    } else {
      foreach pitch_max $pitch_list {
        if {$pitch < $pitch_max} {
          return [dict get $data $pitch_max]
        }
      }
    }
    return {}
  }

  proc get_footprint_bump_pitch {} {
    variable footprint
    variable def_units
    
    if {![dict exists $footprint scaled bump_pitch]} {
      if {[dict exists $footprint bump pitch]} {
        dict set footprint scaled bump_pitch [expr round([dict get $footprint bump pitch] * $def_units)]
      } else {
        dict set footprint scaled bump_pitch [get_library_bump_pitch]
      }
    }
    return [dict get $footprint scaled bump_pitch]
  }

  proc get_footprint_bump_width {} {
    variable footprint
    variable def_units
    
    if {![dict exists $footprint scaled bump_width]} {
      if {[dict exists $footprint bump width]} {
        dict set footprint scaled bump_width [expr round([dict get $footprint bump width] * $def_units)]
      } else {
        dict set footprint scaled bump_width [get_library_bump_width]
      }
    }
    return [dict get $footprint scaled bump_width]
  }

  proc get_footprint_rdl_width {} {
    variable footprint 
    variable def_units
    
    if {![dict exists $footprint scaled rdl_width]} {
      if {[dict exists $footprint rdl width]} {
        dict set footprint scaled rdl_width [expr round([dict get $footprint rdl width] * $def_units)]
      } else {
        dict set footprint scaled rdl_width [get_library_rdl_width]
      }
    }
    
    return [dict get $footprint scaled rdl_width]
  }

  proc get_footprint_rdl_spacing {} {
    variable footprint 
    variable def_units
    
    if {![dict exists $footprint scaled rdl_spacing]} {
      if {[dict exists $footprint rdl spacing]} {
        dict set footprint scaled rdl_spacing [expr round([dict get $footprint rdl spacing] * $def_units)]
      } else {
        dict set footprint scaled rdl_spacing [get_library_rdl_spacing]
      }
    }
    
    return [dict get $footprint scaled rdl_spacing]
  }

  proc get_library_bump_pitch {} {
    variable library
    variable def_units
    
    if {![dict exists $library scaled bump_pitch]} {
      if {[dict exists $library bump pitch]} {
        dict set library scaled bump_pitch [expr round([dict get $library bump pitch] * $def_units)]
      } else {
        utl::error "PAD" 35 "No bump_pitch defined in library data"
      }
    }
    return [dict get $library scaled bump_pitch]
  }
  
  proc get_library_bump_width {} {
    variable library
    variable def_units
    
    if {![dict exists $library scaled bump_width]} {
      if {[dict exists $library bump width]} {
        dict set library scaled bump_width [expr round([dict get $library bump width] * $def_units)]
      } else {
        if {[dict exists $library bump cell_name]} {
          set cell_name [lookup_by_bump_pitch [dict get $library bump cell_name]]
          if  {[dict exists $library cells $cell_name width]} {
            dict set library scaled bump_width [expr round([dict get $library cells $cell_name width] * $def_units)]
          } else {
            utl::error "PAD" 36 "No width defined for selected bump cell $cell_name"
          }
        } else {
          utl::error "PAD" 37 "No bump_width defined in library data"
        }
      }
    }
    return [dict get $library scaled bump_width]
  }
  
  proc get_library_bump_pin_name {} {
    variable library
    variable def_units
    
    if {![dict exists $library bump pin_name]} {
      utl::error "PAD" 38 "No bump_pin_name attribute found in the library"
    }
    return [dict get $library bump pin_name]
  }
  
  proc get_library_rdl_width {} {
    variable library
    variable def_units

    if {![dict exists $library scaled rdl_width]} {
      if {[dict exists $library rdl width]} {
        dict set library scaled rdl_width [expr round([dict get $library rdl width] * $def_units)]
      } else {
        utl::error "PAD" 39 "No rdl_width defined in library data"
      }
    }
    return [dict get $library scaled rdl_width]
  }

  proc get_library_rdl_spacing {} {
    variable library
    variable def_units

    if {![dict exists $library scaled rdl_spacing]} {
      if {[dict exists $library rdl spacing]} {
        dict set library scaled rdl_spacing [expr round([dict get $library rdl spacing] * $def_units)]
      } else {
        utl::error "PAD" 40 "No rdl_spacing defined in library data"
      }
    }
    return [dict get $library scaled rdl_spacing]
  }

  proc get_footprint_rdl_cover_file_name {} {
    variable footprint

    if {![dict exists $footprint rdl_cover_file_name]} {
      dict set footprint rdl_cover_file_name "cover.def"
    }

    return [dict get $footprint rdl_cover_file_name]
  }

  proc get_footprint_rdl_layer_name {} {
    variable footprint
    
    if {![dict exists $footprint rdl_layer_name]} {
      dict set footprint rdl_layer_name [get_library_rdl_layer_name]
    }
    
    return [dict get $footprint rdl_layer_name]
  }
  
  proc get_library_rdl_layer_name {} {
    variable library
    
    if {![dict exists $library rdl_layer_name]} {
      dict set library rdl_layer_name [lindex $pdngen::metal_layers end]
    }
    
    return [dict get $library rdl_layer_name]
  }
  
  proc get_footprint_pads_per_pitch {} {
    variable footprint 
    
    return [dict get $footprint pads_per_pitch]
  }
  
  proc get_footprint_corner_size {} {
    variable library
    
    set pads_per_pitch [lookup_by_bump_pitch [dict get $library num_pads_per_tile]]
    return $pads_per_pitch
  }
  
  proc is_power_net {net_name} {
    variable footprint

    if {[dict exists $footprint power_nets]} {
      if {[lsearch [dict get $footprint power_nets] $net_name] > -1} {
        return 1
      }
    } 

    return 0
  } 

  proc is_ground_net {net_name} {
    variable footprint

    if {[dict exists $footprint ground_nets]} {
      if {[lsearch [dict get $footprint ground_nets] $net_name] > -1} {
        return 1
      }
    } 

    return 0
  }

  # Trace 0 goes directly up to bump 0
  proc path_trace_0 {x y} {
    upvar tile_offset_y tile_offset_y
    upvar tile_width tile_width
    
    set path [list \
      [list $x $y] \
      [list $x [expr $tile_offset_y + $tile_width / 2]] \
    ]
    
    return $path
  }

  # Trace 1 goes up the LHS of bump 0 to bump 1
  proc path_trace_1 {x y} {
    upvar tile_offset_y tile_offset_y
    upvar tile_width tile_width
    upvar rdl_width rdl_width
    upvar rdl_spacing rdl_spacing
    upvar bump_width bump_width
    upvar bump_edge_width bump_edge_width
    upvar offset offset 
    upvar y_min y_min
    upvar y_max y_max

    set path [list \
      [list $x $y] \
      [list $x [expr $y_min(1) - abs($x - $offset(1))]] \
      [list $offset(1) $y_min(1)] \
      [list $offset(1) $y_max(1)] \
      [list [expr $tile_width / 2] [expr $tile_width + $tile_offset_y + $tile_width / 2]] \
    ]
    
    return $path
  }

  # Trace 2 goes up the RHS of bump 0 to bump 2
  proc path_trace_2 {x y} {
    upvar tile_offset_y tile_offset_y
    upvar tile_width tile_width
    upvar rdl_width rdl_width
    upvar rdl_spacing rdl_spacing
    upvar bump_width bump_width
    upvar bump_edge_width bump_edge_width
    upvar offset offset
    upvar y_min y_min
    upvar y_max y_max
    
    set path [list \
      [list $x $y] \
      [list $x [expr $y_min(2) - abs($x - $offset(2))]] \
      [list $offset(2) $y_min(2)] \
      [list $offset(2) $y_max(2)] \
      [list [expr $tile_width / 2] [expr 2 * $tile_width + $tile_offset_y + $tile_width / 2]] \
    ]
    
    return $path
  }

  # Trace 3 goes up the LHS of bump 0 to bump 3
  proc path_trace_3 {x y} {
    upvar tile_offset_y tile_offset_y
    upvar tile_width tile_width
    upvar rdl_width rdl_width
    upvar rdl_spacing rdl_spacing
    upvar bump_width bump_width
    upvar bump_edge_width bump_edge_width
    upvar offset offset 
    upvar y_min y_min
    upvar y_max y_max
    
    set path [list \
      [list $x $y] \
      [list $x [expr $y_min(3) - abs($x - $offset(3))]] \
      [list $offset(3) $y_min(3)] \
      [list $offset(3) $y_max(3)] \
      [list [expr $tile_width / 2] [expr 3 * $tile_width + $tile_offset_y + $tile_width / 2]] \
    ]
    
    return $path
  }

  # Trace 4 goes up the RHS of bump 0 to bump 4
  proc path_trace_4 {x y} {
    upvar tile_offset_y tile_offset_y
    upvar tile_width tile_width
    upvar rdl_width rdl_width
    upvar rdl_spacing rdl_spacing
    upvar bump_width bump_width
    upvar bump_edge_width bump_edge_width
    upvar offset offset 
    upvar y_min y_min
    upvar y_max y_max
    
    set path [list \
      [list $x $y] \
      [list $x [expr $y_min(4) - abs($x - $offset(4))]] \
      [list $offset(4) $y_min(4)] \
      [list $offset(4) $y_max(4)] \
      [list [expr $tile_width / 2] [expr 4 * $tile_width + $tile_offset_y + $tile_width / 2]] \
    ]
    
    return $path
  }

  proc transform_point {x y origin orientation} {
    switch -exact $orientation {
      R0    {set new_point [list $x $y]}
      R90   {set new_point [list [expr -1 * $y] $x]}
      R180  {set new_point [list [expr -1 * $x] [expr -1 * $y]]}
      R270  {set new_point [list $y [expr -1 * $x]]}
      MX    {set new_point [list $x [expr -1 * $y]]}
      MY    {set new_point [list [expr -1 * $x] $y]}
      MXR90 {set new_point [list $y $x]}
      MYR90 {set new_point [list [expr -1 * $y] [expr -1 * $x]]}
      default {utl::error "PAD" 27 "Illegal orientation $orientation specified"}
    }
    return [list \
      [expr [lindex $new_point 0] + [lindex $origin 0]] \
      [expr [lindex $new_point 1] + [lindex $origin 1]] \
    ]
  }

  proc invert_transform {x y origin orientation} {
    set x [expr $x - [lindex $origin 0]]
    set y [expr $y - [lindex $origin 1]]

    switch -exact $orientation {
      R0    {set new_point [list $x $y]}
      R90   {set new_point [list $y [expr -1 * $x]]}
      R180  {set new_point [list [expr -1 * $x] [expr -1 * $y]]}
      R270  {set new_point [list [expr -1 * $y] $x]}
      MX    {set new_point [list $x [expr -1 * $y]]}
      MY    {set new_point [list [expr -1 * $x] $y]}
      MXR90 {set new_point [list $y $x]}
      MYR90 {set new_point [list [expr -1 * $y] [expr -1 * $x]]}
      default {utl::error "PAD" 28 "Illegal orientation $orientation specified"}
    }

    return $new_point
  }

  proc transform_path {path origin orientation} {
    set new_path {}

    foreach point $path {
      lappend new_path [transform_point {*}$point $origin $orientation]
    }

    return $new_path
  }

  proc get_side {row col} {
    variable num_bumps_x
    variable num_bumps_y
    
    set corner_size [get_footprint_corner_size]
    set row_n [expr $num_bumps_y - $row + 1]
    set col_n [expr $num_bumps_x - $col + 1]

    if {$row <= $corner_size} {
      if {$row >= $col} {return "l"} 
      if {$row > $col_n} {return "r"}
      return "t"
    }
    
    if {$row_n <= $corner_size} {
      if {$row_n > $col} {return "l"} 
      if {$row_n >= $col_n} {return "r"}
      return "b"
    }

    if {$col <= $corner_size} {
      return "l"
    }
    
    if {$col_n <= $corner_size} {
      return "r"
    }
    
    return "c"
  }

  proc connect_bumps_to_padcells {} {
    variable num_bumps_x
    variable num_bumps_y
    variable actual_tile_offset_x
    variable actual_tile_offset_y

    # debug "start: ($num_bumps_x, $num_bumps_y)"
    
    set corner_size [get_footprint_corner_size]
    
    set bump_pitch [get_footprint_bump_pitch]
    set bump_width [get_footprint_bump_width]
    set tile_width $bump_pitch

    set rdl_width [get_footprint_rdl_width]
    set rdl_spacing [get_footprint_rdl_spacing]
    
    variable tile_offset_x $actual_tile_offset_x
    variable tile_offset_y $actual_tile_offset_y

    set tile_width [get_footprint_bump_pitch]
    
    set offset(1) [expr ($rdl_width + $rdl_spacing) * 3 / 2]
    set offset(2) [expr $tile_width - ($rdl_width + $rdl_spacing) * 3 / 2]
    set offset(3) [expr ($rdl_width + $rdl_spacing) / 2]
    set offset(4) [expr $tile_width - ($rdl_width + $rdl_spacing) / 2]

    set y_max(1)   [expr $tile_offset_y + $tile_width / 2 + 1 * $bump_pitch - abs($tile_width / 2 - $offset(1))]
    set y_max(2)   [expr $tile_offset_y + $tile_width / 2 + 2 * $bump_pitch - abs($tile_width / 2 - $offset(2))]
    set y_max(3)   [expr $tile_offset_y + $tile_width / 2 + 3 * $bump_pitch - abs($tile_width / 2 - $offset(3))]
    set y_max(4)   [expr $tile_offset_y + $tile_width / 2 + 4 * $bump_pitch - abs($tile_width / 2 - $offset(4))]

    set y_min(1) [expr $tile_offset_y + $tile_width / 2 - $bump_width / 2 - ($rdl_width / 2 + $rdl_spacing)]
    set y_min(2) [expr $tile_offset_y + $tile_width / 2 - $bump_width / 2 - ($rdl_width / 2 + $rdl_spacing)]
    set y_min(3) [expr $tile_offset_y + $tile_width / 2 - $bump_width / 2 - (3 * $rdl_width / 2 + 2 * $rdl_spacing)]
    set y_min(4) [expr $tile_offset_y + $tile_width / 2 - $bump_width / 2 - (3 * $rdl_width / 2 + 2 * $rdl_spacing)]

    set die_area [get_scaled_die_area]
    
    # Bottom side
    for {set row 1} {$row <= $num_bumps_x} {incr row} {
      for {set col 1} {$col <= $num_bumps_y} {incr col} {
        if {$row > $corner_size && ($num_bumps_y - $row >= $corner_size) && $col > $corner_size && ($num_bumps_x - $col >= $corner_size)} {
          continue
        }

        set padcell [get_padcell_at_row_col $row $col]
        if {[is_padcell_unassigned $padcell]} {continue}

        set side [get_side $row $col]

        switch $side {
          "b" {
            set orientation "R0"
            set tile_origin [list [expr $actual_tile_offset_x + ($col - 1) * $tile_width] [lindex $die_area 1]]
            set trace_func "path_trace_[expr $num_bumps_y - $row]"
          }
          "r" {
            set orientation "R90"
            set tile_origin [list [lindex $die_area 2] [expr $actual_tile_offset_y + ($num_bumps_y - $row) * $tile_width]]
            set trace_func "path_trace_[expr $num_bumps_x - $col]"
          }
          "t" {
            set orientation "R180"
            set tile_origin [list [expr $actual_tile_offset_x + $col * $tile_width] [lindex $die_area 3]]
            set trace_func "path_trace_[expr $row - 1]"
          }
          "l" {
            set orientation "R270"
            set tile_origin [list [lindex $die_area 0] [expr $actual_tile_offset_y + ($num_bumps_y - $row + 1) * $tile_width]]
            set trace_func "path_trace_[expr $col - 1]"
          }
        }
        
        set padcell_pin_centre [get_box_centre [get_padcell_pad_pin_shape $padcell]]
        set path [transform_path [$trace_func {*}[invert_transform {*}$padcell_pin_centre $tile_origin $orientation]] $tile_origin $orientation]
        
        set_padcell_rdl_trace $padcell $path
      }
    }
  }

  proc write_rdl_trace_def {} {
    variable num_bumps_x
    variable num_bumps_y

    set rdl_cover_file_name [get_footprint_rdl_cover_file_name]
    set rdl_layer_name [get_footprint_rdl_layer_name]
    set rdl_width [get_footprint_rdl_width]

    set ch [open $rdl_cover_file_name "w"]
    set traces {} 
    for {set row 1} {$row <= $num_bumps_y} {incr row} {
      for {set col 1} {$col <= $num_bumps_x} {incr col} {
        # debug "($row, $col)"
        if {[set padcell [get_padcell_at_row_col $row $col]] == ""} {continue}
        if {[is_padcell_unassigned $padcell]} {continue}
        
        if {[dict exists $traces [get_padcell_pin_name $padcell]]} {
          dict set traces [get_padcell_pin_name $padcell] [concat [dict get $traces [get_padcell_pin_name $padcell]] $padcell]
        } else {
          dict set traces [get_padcell_pin_name $padcell] $padcell
        }
      }
    }

    puts $ch "SPECIALNETS [dict size $traces] ;"
    
    dict for {net padcells} $traces {
      puts $ch "    - $net "
      if {[is_padcell_power [lindex $padcells 0]]} {
        puts $ch "      + USE POWER"
      }
      if {[is_padcell_ground [lindex $padcells 0]]} {
        puts $ch "      + USE GROUND"
      }
      set prefix "+ ROUTED"
      foreach padcell $padcells {
        puts -nonewline $ch "      $prefix $rdl_layer_name $rdl_width "
        foreach point [get_padcell_rdl_trace $padcell] {
          puts -nonewline $ch " ( [lindex $point 0] [lindex $point 1] )"
        }
        puts $ch ""
        set prefix "NEW"
      }
      puts $ch "      ;"
    }

    puts $ch "END SPECIALNETS"

    close $ch
  }
  
  proc place_bumps {} {
    variable num_bumps_x
    variable num_bumps_y
    variable block

    set corner_size [get_footprint_corner_size]
    set bump_master [get_cell bump]
    for {set row 1} {$row <= $num_bumps_y} {incr row} {
      for {set col 1} {$col <= $num_bumps_x} {incr col} {
        set origin [get_bump_origin $row $col]
        set bump_name [get_bump_name_at_row_col $row $col]
        # debug "bump ($row $col): $bump_name"

        set inst [odb::dbInst_create $block $bump_master $bump_name]

        $inst setOrigin [dict get $origin x] [dict get $origin y]
        $inst setOrient "R0"
        $inst setPlacementStatus "FIRM"
        
        set padcell [get_padcell_at_row_col $row $col]
        if {$padcell != ""} {
          connect_to_bondpad_or_bump $inst $origin $padcell
        }
      }
    }
    # debug "end"
  }

  proc add_power_ground_rdl_straps {} {
    variable num_bumps_x
    variable num_bumps_y

    set rdl_routing_layer [get_footprint_rdl_layer_name]
    set rdl_stripe_width [get_footprint_rdl_width]
    set rdl_stripe_spacing [get_footprint_rdl_spacing]
    set corner_size [get_footprint_corner_size]
    set bump_pitch [get_footprint_bump_pitch]
    
    # debug "rdl_routing_layer $rdl_routing_layer"
    # debug "rdl_stripe_width $rdl_stripe_width"
    # debug "rdl_stripe_spacing $rdl_stripe_spacing"
    # debug "corner_size $corner_size"
    # debug "bump_pitch $bump_pitch"
    
    # Add stripes for bumps in the central core area
    if {[pdngen::get_dir $rdl_routing_layer] == "hor"} {
      set point [get_bump_centre [expr $corner_size + 1] 1]
      set minX [expr [dict get $point x] - $bump_pitch / 2]
      set point [get_bump_centre [expr $num_bumps_x - $corner_size] 1]
      set maxX [expr [dict get $point x] + $bump_pitch / 2]

      for {set row [expr $corner_size + 1]} {$row <= $num_bumps_y - $corner_size} {incr row} {
        if {$row % 2 == 0} {
          set tag "POWER"
        } else {
          set tag "GROUND"
        }
        set y [dict get [get_bump_centre $row 1] y]
        set lowerY [expr $y - ($bump_pitch - $rdl_stripe_width - $rdl_stripe_spacing) / 2]
        set upperY [expr $y + ($bump_pitch - $rdl_stripe_width - $rdl_stripe_spacing) / 2]
        set upper_stripe [odb::newSetFromRect $minX [expr $upperY - $rdl_stripe_width / 2] $maxX [expr $upperY + $rdl_stripe_width / 2]]
        set lower_stripe [odb::newSetFromRect $minX [expr $lowerY - $rdl_stripe_width / 2] $maxX [expr $lowerY + $rdl_stripe_width / 2]]
        pdngen::add_stripe $rdl_routing_layer $tag $upper_stripe
        pdngen::add_stripe $rdl_routing_layer $tag $lower_stripe
        for {set col [expr $corner_size + 1]} {$col <= $num_bumps_x - $corner_size} {incr col} {
          set point [get_bump_centre $row $col]
          set link_stripe [odb::newSetFromRect [expr [dict get $point x] - $rdl_stripe_width / 2] $lowerY [expr [dict get $point x] + $rdl_stripe_width / 2] $upperY]
          pdngen::add_stripe $rdl_routing_layer $tag $link_stripe
        }
      }
    } elseif {[pdngen::get_dir $rdl_routing_layer] == "ver"} {
      set point [get_bump_centre 1 [expr $corner_size + 1]]
      set minY [expr [dict get $point x] - $bump_pitch / 2]
      set point [get_bump_centre 1 [expr $num_bumps_y - $corner_size]]
      set maxY [expr [dict get $point x] + $bump_pitch / 2]

      for {set col [expr $corner_size + 1]} {$col <= $num_bumps_x - $corner_size} {incr col} {
        if {$col % 2 == 0} {
          set tag "POWER"
        } else {
          set tag "GROUND"
        }
        set x [dict get [get_bump_centre 1 $col] x]
        set lowerX [expr $x - ($bump_pitch - $rdl_stripe_width - $rdl_stripe_spacing) / 2]
        set upperX [expr $x + ($bump_pitch - $rdl_stripe_width - $rdl_stripe_spacing) / 2]
        set upper_stripe [odb::newSetFromRect [expr $upperX - $rdl_stripe_width / 2] $minY [expr $upperX + $rdl_stripe_width / 2] $maxY]
        set lower_stripe [odb::newSetFromRect [expr $lowerX - $rdl_stripe_width / 2] $minY [expr $lowerX + $rdl_stripe_width / 2] $maxY]
        pdngen::add_stripe $rdl_routing_layer $tag $upper_stripe
        pdngen::add_stripe $rdl_routing_layer $tag $lower_stripe
        for {set row [expr $corner_size + 1]} {$row <= $num_bumps_y - $corner_size} {incr row} {
          set point [get_bump_centre $row $col]
          set link_stripe [odb::newSetFromRect $lowerX [expr [dict get $point y] - $rdl_stripe_width / 2] $upperX [expr [dict get $point y] + $rdl_stripe_width / 2]]
          pdngen::add_stripe $rdl_routing_layer $tag $link_stripe
        }
      }
    }
    
    # debug "$pdngen::metal_layers"
    # debug "[array get pdngen::stripe_locs]"
    pdngen::merge_stripes
    dict set pdngen::design_data power_nets "VDD"
    dict set pdngen::design_data ground_nets "VSS"
    pdngen::opendb_update_grid
  }
    # Connect up pads in the corner regions
    # Connect group of padcells up to the column of allocated bumps

  proc place_padring {} {
    variable block
    variable tech
    variable chip_width 
    variable chip_height
    variable edge_bottom_offset 
    variable edge_right_offset 
    variable edge_top_offset 
    variable edge_left_offset
    variable pad_ring

    place_corners
 
    foreach side_name {bottom right top left} {
      switch $side_name \
        "bottom" {
          set fill_start [expr $edge_left_offset + [corner_width corner_ll]]
          set fill_end   [expr $chip_width - $edge_right_offset - [corner_width corner_lr]]
        } \
        "right"  {
          set fill_start [expr $edge_bottom_offset + [corner_height corner_lr]]
          set fill_end   [expr $chip_height - $edge_top_offset - [corner_height corner_ur]]
        } \
        "top"    {
          set fill_start [expr $chip_width - $edge_right_offset - [corner_width corner_ur]]
          set fill_end   [expr $edge_left_offset + [corner_width corner_ul]]
        } \
        "left"   {
          set fill_start [expr $chip_height - $edge_top_offset - [corner_height corner_ul]]
          set fill_end   [expr $edge_bottom_offset + [corner_height corner_ll]]
        }

      # debug "$side_name: fill_start = $fill_start"
      # debug "$side_name: fill_end   = $fill_end"
      # debug "padcells: [get_footprint_padcells_by_side $side_name]"

      set bbox {}
      set padcells_on_side [get_footprint_padcells_by_side $side_name]
      if {[llength $padcells_on_side] == 0} {
        ord::error "PAD" 98 "No cells found on $side side"
      }
      foreach padcell [get_footprint_padcells_by_side $side_name] {
        set name [get_padcell_inst_name $padcell]
        set type [get_padcell_type $padcell]
        set cell [get_cell $type $side_name]
        # debug "name: $name, type: $type, cell: $cell"

        set cell_height [expr max([$cell getHeight],[$cell getWidth])]
        set cell_width  [expr min([$cell getHeight],[$cell getWidth])]

        if {[set inst [get_padcell_inst $padcell]] == "NULL"} {
          # utl::warn "PAD" 13 "Expected instance $name for signal $name not found"
          continue
        }


        set orient [get_padcell_orient $padcell]
	set origin [get_padcell_origin $padcell]
        # debug "padcell: $padcell, orient: $orient, origin: $origin"
        # set offset [get_padcell_cell_offset $padcell]
        # set location [transform_point {*}$offset [list 0 0] $orient]
        # set place_at [list [expr [dict get $origin x] - [lindex $location 0]] [expr [dict get $origin y] - [lindex $location 1]]]
        set place_at [list [dict get $origin x] [dict get $origin y]]
        # debug "padcell: $padcell, origin: $origin, orient: $orient, place_at: $place_at"

        $inst setOrigin {*}$place_at
        $inst setOrient $orient
        $inst setPlacementStatus "FIRM"
        place_padcell_overlay $padcell

        # debug "inst: [$inst getName]"
        set bbox [$inst getBBox]

        switch $side_name \
          "bottom" {
            # debug "cell_width: $cell_width cell_height: $cell_height"
            fill_box $fill_start [$bbox yMin] [$bbox xMin] [$bbox yMax] $side_name
            set fill_start [$bbox xMax]
          } \
          "right"  {
            # debug "fill after [$inst getName]"
            fill_box [$bbox xMin] $fill_start [$bbox xMax] [$bbox yMin] $side_name
            set fill_start [$bbox yMax]
          } \
          "top" {
            fill_box [$bbox xMax] [$bbox yMin] $fill_start [$bbox yMax] $side_name
            # debug "added_cell: [$inst getName] ($x $y) [$cell getName] [$cell getWidth] x [$cell getHeight]"
            set fill_start [$bbox xMin]
            # debug "$side_name: fill_start = $fill_start"
          } \
          "left" {
            fill_box [$bbox xMin] [$bbox yMax] [$bbox xMax] $fill_start $side_name
            set fill_start [$bbox yMin]
          }


        dict set pad_ring $name $inst
        dict incr idx $type
      } 

      # debug "$side_name: fill_start = $fill_start"
      # debug "$side_name: fill_end   = $fill_end"
      switch $side_name \
        "bottom" {
          fill_box $fill_start [$bbox yMin] $fill_end [$bbox yMax] $side_name
        } \
        "right"  {
          fill_box [$bbox xMin] $fill_start [$bbox xMax] $fill_end $side_name
        } \
        "top" {
          fill_box $fill_end [$bbox yMin] $fill_start [$bbox yMax] $side_name
        } \
        "left" {
          fill_box [$bbox xMin] $fill_end [$bbox xMax] $fill_start $side_name
        }

    }
  }

  proc corner_width {corner} {
    variable pad_ring

    set inst [dict get $pad_ring $corner]
    return [[$inst getBBox] getDX]
  }

  proc corner_height {corner} {
    variable pad_ring

    set inst [dict get $pad_ring $corner]
    return [[$inst getBBox] getDY]
  }

  proc place_corners {} {
    variable block
    variable pad_ring
    variable chip_width 
    variable chip_height
    variable edge_bottom_offset 
    variable edge_right_offset 
    variable edge_top_offset 
    variable edge_left_offset

    dict set pad_ring corner_ll [set inst [odb::dbInst_create $block [set corner [get_cell corner ll]] "CORNER_LL"]]
    set cell_offset [get_library_cell_type_offset corner]
    set corner_orient [get_library_cell_orientation [$corner getName] ll]
    set corner_origin [list $edge_left_offset $edge_bottom_offset]
    set location [transform_point {*}$cell_offset [list 0 0] $corner_orient]
    set place_at [list [expr [lindex $corner_origin 0] - [lindex $location 0]] [expr [lindex $corner_origin 1] - [lindex $location 1]]]
    # debug "LL: place - $corner_origin, offset - $cell_offset, location - $location, orient - [get_library_cell_orientation [$corner getName] ll]"
    $inst setOrigin {*}$place_at
    $inst setOrient $corner_orient
    $inst setPlacementStatus "FIRM"

    dict set pad_ring corner_lr [set inst [odb::dbInst_create $block [set corner [get_cell corner lr]] "CORNER_LR"]]
    set cell_offset [get_library_cell_type_offset corner]
    set corner_orient [get_library_cell_orientation [$corner getName] lr]
    set corner_origin [list [expr ($chip_width - $edge_right_offset)] $edge_bottom_offset]
    set location [transform_point {*}$cell_offset [list 0 0] $corner_orient]
    set place_at [list [expr [lindex $corner_origin 0] - [lindex $location 0]] [expr [lindex $corner_origin 1] - [lindex $location 1]]]
    # debug "LR: place - $corner_origin, offset - $cell_offset, location - $location, orient - [get_library_cell_orientation [$corner getName] lr]"
    $inst setOrigin {*}$place_at
    $inst setOrient $corner_orient
    $inst setPlacementStatus "FIRM"

    dict set pad_ring corner_ur [set inst [odb::dbInst_create $block [set corner [get_cell corner ur]] "CORNER_UR"]]
    set cell_offset [get_library_cell_type_offset corner]
    set corner_orient [get_library_cell_orientation [$corner getName] ur]
    set corner_origin [list [expr ($chip_width - $edge_right_offset)] [expr ($chip_height - $edge_top_offset)]]
    set location [transform_point {*}$cell_offset [list 0 0] $corner_orient]
    set place_at [list [expr [lindex $corner_origin 0] - [lindex $location 0]] [expr [lindex $corner_origin 1] - [lindex $location 1]]]
    # debug "UR: place - $corner_origin, offset - $cell_offset, location - $location, orient - [get_library_cell_orientation [$corner getName] ur]"
    $inst setOrigin {*}$place_at
    $inst setOrient $corner_orient
    $inst setPlacementStatus "FIRM"

    dict set pad_ring corner_ul [set inst [odb::dbInst_create $block [set corner [get_cell corner ul]] "CORNER_UL"]]
    set cell_offset [get_library_cell_type_offset corner]
    set corner_orient [get_library_cell_orientation [$corner getName] ul]
    set corner_origin [list $edge_left_offset [expr ($chip_height - $edge_top_offset)]]
    set location [transform_point {*}$cell_offset [list 0 0] $corner_orient]
    set place_at [list [expr [lindex $corner_origin 0] - [lindex $location 0]] [expr [lindex $corner_origin 1] - [lindex $location 1]]]
    # debug "UL: place - $corner_origin, offset - $cell_offset, location - $location, orient - [get_library_cell_orientation [$corner getName] ul]"
    $inst setOrigin {*}$place_at
    $inst setOrient $corner_orient
    $inst setPlacementStatus "FIRM"
  }  
  
  proc place_additional_cells {} {
    variable block
    variable footprint

    if {[dict exists $footprint place]} {
      dict for {cell_name inst_info} [dict get $footprint place] {
        # debug "cell_name $cell_name"
        set name [dict get $inst_info name]
        set type [dict get $inst_info type]
        # debug "name $name, type $type"
        set cell [get_cell $type]
        # debug "cell: $cell"
        
        if {[set inst [$block findInst $name]] == "NULL"} {
          set inst [odb::dbInst_create $block $cell $name]
        }

        if {$inst == "NULL"} {
          set str "init_footprint: inst_info $inst_info"
          set str "$str\ninit_footprint: name $name"
          set str "$str\ninit_footprint: type $type"
          set str "$str\ninit_footprint: cell $cell"
          utl::error "PAD" 3 "$str\nCannot create instance for $cell_name"
        }

        set origin [get_scaled_origin $cell_name cell]

        $inst setOrigin [dict get $origin x] [dict get $origin y]
        $inst setOrient [dict get $inst_info cell orient]
        $inst setPlacementStatus "FIRM"
      }
    }
  }

  proc connect_by_abutment {} {
    variable library
    variable block

    if {[dict exists $library connect_by_abutment]} {
      # Breaker cells affect the connectivity of signals connected by abutment.
      set segment {}
      array set pad_segment {}
      set breaker_types {}
      if {[dict exists $library breakers]} {
        set breaker_types [dict get $library breakers]
      }

      # Start each signal index at 0, and increment each time a breaker breaks the abutted signal
      foreach signal [dict get $library connect_by_abutment] {
        dict set segment $signal cur_index 0
        set pad_segment($signal,0) {}
      }

      foreach padcell [get_footprint_padcell_order] {
        set side_name [get_padcell_side_name $padcell]

        # debug "padcell: $padcell, inst [get_padcell_inst $padcell]"
        if {[set inst [get_padcell_inst $padcell]] == "NULL"} {
          set name [get_padcell_inst_name $padcell]
        } else {
          set name [$inst getName]
        }
        set type [get_padcell_type $padcell]

        if {[set brk_idx [lsearch $breaker_types $type]] > -1} {
          set breaker [dict get $library types [lindex $breaker_types $brk_idx]]
          set signal_breaks [dict get $library cells $breaker breaks]
          foreach signal [dict get $library connect_by_abutment] {
            if {[lsearch [dict keys $signal_breaks] $signal] > -1} {
              set cur_index [dict get $segment $signal cur_index]
              
              set before_pin [lindex [dict get $signal_breaks $signal] 0]
              set after_pin [lindex [dict get $signal_breaks $signal] 1]
             
              dict set segment breaker $signal $cur_index $name [list before_pin $before_pin after_pin $after_pin]
              set cur_index [expr $cur_index + 1]
              
              dict set segment $signal cur_index $cur_index
              set pad_segment($signal,$cur_index) {}
            } else {
              set cell [dict get $library types $type]
              set pin_name $signal
              if {[dict exists $library cells $cell connect $signal]} {
                set pin_name [dict get $library cells $cell connect $signal]
              }
              # debug "Adding inst_name $name pin_name $pin_name"
              # Dont add breakers into netlist, as they are physical_only
              # lappend pad_segment($signal,[dict get $segment $signal cur_index]) [list inst_name $name pin_name $pin_name]
            }
          }
        } else {
          foreach signal [dict get $library connect_by_abutment] {
            set cell [dict get $library types $type]
            if {$type == "fill"} {continue} 
            if {$type == "corner"} {continue} 
            set pin_name $signal
            if {[dict exists $library cells $cell connect $signal]} {
              set pin_name [dict get $library cells $cell connect $signal]
            }
            # debug "Adding inst_name $name pin_name $pin_name"
            lappend pad_segment($signal,[dict get $segment $signal cur_index]) [list inst_name $name pin_name $pin_name]
          }
        }
      }
      foreach item [array names pad_segment] {
        # debug "pad_segment: $name"
        regexp {([^,]*),(.*)} $item - signal idx
        dict set segment cells $signal $idx $pad_segment($item)
      }

      # If there is more than one segment, need to account for the first and last segment to be the
      # same, as they loop around back to the start
      dict for {signal seg} [dict get $segment cells] {
        set indexes [lsort -integer [dict keys $seg]]
        set first [lindex $indexes 0]
        set last [lindex $indexes end]
        
        # debug "Signal: $signal, first: $first, last: $last"
        if {$first != $last} {
          dict set seg $first [concat [dict get $seg $first] [dict get $seg $last]]
          dict set segment cells $signal [dict remove $seg $last]
        }
      }

      # Wire up the nets that connect by abutment
      # Need to set these nets as SPECIAL so the detail router does not try to route them.
      dict for {signal sections} [dict get $segment cells] {
        # debug "Signal: $signal"
        foreach section [dict keys $sections] {
          # debug "Section: $section"
          if {[set net [$block findNet "${signal}_$section"]] != "NULL"} {
            # utl::error "PAD" 14 "Net ${signal}_$section already exists, so cannot be used in the pad ring"
          } else {
            set net [odb::dbNet_create $block "${signal}_$section"]
          }
          # debug "Net: [$net getName]"
          $net setSpecial

          foreach inst_pin_name [dict get $sections $section] {
            set inst_name [dict get $inst_pin_name inst_name]
            set pin_name [dict get $inst_pin_name pin_name]

            if {[set inst [$block findInst $inst_name]] == "NULL"} {
              # debug "Cant find instance $inst_name"
              continue
            }
            # debug "inst_name: $inst_name, pin_name: $pin_name"    
            set mterm [[$inst getMaster] findMTerm $pin_name]
            if {$mterm != "NULL"} {
              set iterm [odb::dbITerm_connect $inst $net $mterm]
              # debug "connect"
              $iterm setSpecial
            } else {
              utl::warn "PAD" 18 "No term $signal found on $inst_name"
            }
          }
        }
      }
    }
  }
  
  proc global_assignments {} {
    variable library

    # Determine actual parameter values for each pad instance
    # Use separate namespaces to ensure there is no clash in evaluation
    if {[namespace exists pad_inst]} {
      namespace delete pad_inst
    }
    namespace eval pad_inst {}
    # Evaluate all the parameters for all padcell instances
    foreach padcell [get_footprint_padcell_order] {
      # debug "padcell - $padcell"
      set type [get_padcell_type $padcell]
      set library_element [dict get $library types $type]
      set library_cell [dict get $library cells $library_element]

      namespace eval "pad_inst_values::$padcell" {}
      if {$type == "sig"} {
      }
      
      if {[dict exists $library_cell default_parameters]} {
        # debug "default_parameters - [dict get $library_cell default_parameters]"
        dict for {parameter value} [dict get $library_cell default_parameters] {
          # debug "pad_inst_values::${padcell}::$parameter"
          # debug "[dict get $library_cell default_parameters $parameter]"
          set "pad_inst_values::${padcell}::$parameter" $value
        }
      }
    }
  }
  
  proc get_library_types {} {
    variable library
    
    if {![dict exists $library types]} {
      utl::error 29 "No types specified in the library"
    }
    
    return [dict keys [dict get $library types]]
  }
  
  variable cover_def "cover.def"
  proc set_cover_def_file_name {file_name} {
    variable cover_def
    
    set cover_def $file_name
  }

  variable signal_assignment_file ""
  proc set_signal_assignment_file {file_name} {
    variable signal_assignment_file
    
    set signal_assignment_file $file_name
  }
  
  proc init_footprint {args} {
    set arglist $args

    if {[set idx [lsearch $arglist "-signal_mapping"]] > -1} {
      set_signal_assignment_file [lindex $arglist [expr $idx + 1]]
      set arglist [lreplace $arglist $idx [expr $idx + 1]]
    }

    if {[llength $arglist] > 1} {
      utl::error "PAD" 30 "Unrecognised arguments to init_footprint $arglist"
    }

    if {[llength $arglist] == 1} {
      set_signal_assignment_file $arglist
    }
    
    pdngen::init_tech

    # Perform signal assignment 
    assign_signals

    # Padring placement
    get_footprint_padcells_order
    place_padcells
    place_padring
    connect_by_abutment

    # Wirebond pad / Flipchip bump connections
    if {[is_footprint_wirebond]} {
      init_library_bondpad
      place_bondpads
      # Bondpads are assumed to connect by abutment
    } elseif {[is_footprint_flipchip]} {
      init_rdl
      # assign_padcells_to_bumps
      place_bumps
      connect_bumps_to_padcells
      add_power_ground_rdl_straps
      write_rdl_trace_def 
    }
    global_assignments  

    # Place miscellaneous other cells
    place_additional_cells
  }
  
  proc add_cell {type name sides} {
    variable cells
    variable default_orientation 
    
    foreach side $sides {
      if {[dict exists $cells $type $side]} {
        set cell_list [dict get $cells $type $side]
      } else {
        set cell_list {}
      }
      lappend cell_list [list master [get_cell_master $name]]
      dict set cells $type $side $cell_list
      
      set_cell_orientation $name $side [dict get $default_orientation $side]
    }
  }
  
  proc set_cell_orientation {name position orient} {
    variable library 
    
    dict set $library cells $name $position orient $orient
  }

  proc is_padcell_power {padcell} {
    return [is_power_net [get_padcell_assigned_name $padcell]]
  }

  proc is_padcell_ground {padcell} {
    return [is_ground_net [get_padcell_assigned_name $padcell]]
  }

  proc is_padcell_physical_only {padcell} {
    variable library 
    
    set type_name [dict get $library types [get_padcell_type $padcell]]
    
    if {[dict exists $library cells $type_name physical_only]} {
      return [dict get $library cells $type_name physical_only]
    } 
    
    return 0
  }
  
  proc is_padcell_control {padcell} {
    variable library 
    
    set type_name [dict get $library types [get_padcell_type $padcell]]
    
    if {[dict exists $library cells $type_name is_control]} {
      return [dict get $library cells $type_name is_control]
    } 
    
    return 0
  }

  proc is_padcell_unassigned {padcell} {
    if {[get_padcell_assigned_name $padcell] == "UNASSIGNED"} {
      return 1
    } else {
      return 0
    }
  }
    
  proc set_offsets {} {
    variable footprint
    variable edge_bottom_offset 
    variable edge_right_offset 
    variable edge_top_offset 
    variable edge_left_offset
    variable def_units

    set args [dict get $footprint offsets]    
    
    if {[llength $args] == 1} {
      set edge_bottom_offset [expr round($args * $def_units)]
      set edge_right_offset  [expr round($args * $def_units)]
      set edge_top_offset    [expr round($args * $def_units)]
      set edge_left_offset   [expr round($args * $def_units)]
    } elseif {[llength $args] == 2} {
      set edge_bottom_offset [expr round([lindex $args 0] * $def_units)]
      set edge_right_offset  [expr round([lindex $args 1] * $def_units)]
      set edge_top_offset    [expr round([lindex $args 0] * $def_units)]
      set edge_left_offset   [expr round([lindex $args 1] * $def_units)]
    } elseif {[llength $args] == 4} {
      set edge_bottom_offset [expr round([lindex $args 0] * $def_units)]
      set edge_right_offset  [expr round([lindex $args 1] * $def_units)]
      set edge_top_offset    [expr round([lindex $args 2] * $def_units)]
      set edge_left_offset   [expr round([lindex $args 3] * $def_units)]
    } else {
      utl::error "PAD" 9 "Expected 1, 2 or 4 offset values, got [llength $args]"
    }
  }

  proc set_inner_offset {args} {
    variable db
    variable inner_bottom_offset 
    variable inner_right_offset 
    variable inner_top_offset 
    variable inner_left_offset
    variable def_units
    
    if {[llength $args] == 1} {
      set inner_bottom_offset [expr $args * $def_units]
      set inner_right_offset  [expr $args * $def_units]
      set inner_top_offset    [expr $args * $def_units]
      set inner_left_offset   [expr $args * $def_units]
    } elseif {[llength $args] == 2} {
      set inner_bottom_offset [expr [lindex $args 0] * $def_units]
      set inner_right_offset  [expr [lindex $args 1] * $def_units]
      set inner_top_offset    [expr [lindex $args 0] * $def_units]
      set inner_left_offset   [expr [lindex $args 1] * $def_units]
    } elseif {[llength $inner_offset] == 4} {
      set inner_bottom_offset [expr [lindex $args 0] * $def_units]
      set inner_right_offset  [expr [lindex $args 1] * $def_units]
      set inner_top_offset    [expr [lindex $args 2] * $def_units]
      set inner_left_offset   [expr [lindex $args 3] * $def_units]
    } else {
      utl::error "PAD" 10 "Expected 1, 2 or 4 inner_offset values, got [llength $args]"
    }
  }
  
  proc init_index {} {
    variable cells
    variable idx
    
    foreach key [dict keys $cells] {
      dict set idx $key 0
    }
  }

  proc fill_box {xmin ymin xmax ymax side} {
    variable cells
    variable idx
    variable edge_bottom_offset 
    variable edge_right_offset 
    variable edge_top_offset 
    variable edge_left_offset
    variable chip_width 
    variable chip_height
    variable block

    set type fill
    
    if {$side == "top" || $side == "bottom"} {
      set fill_start $xmin
      set fill_end $xmax
    } else {
      set fill_start $ymin
      set fill_end $ymax
    }

    set spacers {}
    set filler_cells [get_cells fill $side]
    foreach type_name [dict keys $filler_cells] {
      set cell [dict get $filler_cells $type_name master]
      set width [expr min([$cell getWidth],[$cell getHeight])]
      dict set filler_cells $type_name width $width
      dict set spacers $width $type_name
    }
    set spacer_widths [lreverse [lsort -integer [dict keys $spacers]]]
    set smallest_width [lindex $spacer_widths end]

    while {$fill_start < $fill_end} {

      if {$fill_start + $smallest_width >= $fill_end} {
        # Smallest spacer is larger than available fill space, assume we can use it with overlap
        set spacer_type [dict get $spacers [lindex $spacer_widths end]]
        set spacer_width [dict get $filler_cells $spacer_type width]
        set spacer [dict get $filler_cells $spacer_type master]
      } else {
        # Find the largest spacer that will fit in the gap
        foreach space $spacer_widths {
          set spacer_type [dict get $spacers $space]
          set spacer_width [dict get $filler_cells $spacer_type width]
          if {[expr $fill_start + $spacer_width] <= $fill_end} {
            set spacer [dict get $filler_cells $spacer_type master]
            break
          }
        }
      }

      set name "${type}_[dict get $idx $type]"
      set orient [get_library_cell_orientation $spacer_type $side]
      set inst [odb::dbInst_create $block $spacer $name]
      switch $side \
        "bottom" {
          set x      $fill_start
          set y      $edge_bottom_offset
          set fill_start [expr $x + $spacer_width]
        } \
        "right"  {
          set x      [expr $chip_width - $edge_right_offset]
          set y      $fill_start
          set fill_start [expr $y + $spacer_width]
        } \
        "top"    {
          set x      [expr $fill_start + $spacer_width]
          set y      [expr $chip_height - $edge_top_offset]
          set fill_start $x
        } \
        "left"   {
          set x      $edge_left_offset
          set y      [expr $fill_start + $spacer_width]
          set fill_start $y
        }
      # debug "inst [$inst getName], x: $x, y: $y"
      set offset [get_library_cell_offset $spacer_type]
      set fill_origin [list $x $y]
      set location [transform_point {*}$offset [list 0 0] $orient]
      set place_at [list [expr [lindex $fill_origin 0] - [lindex $location 0]] [expr [lindex $fill_origin 1] - [lindex $location 1]]]
      # debug "offset: $offset, fill_origin: $fill_origin, location: $location, place_at: $place_at"
      $inst setOrigin {*}$place_at 
      $inst setOrient $orient
      $inst setPlacementStatus "FIRM"

      dict incr idx $type
    }
  }
 
  namespace export set_footprint set_library

  namespace export get_die_area get_tracks
  namespace export init_footprint load_footprint
  namespace export get_core_area
  namespace ensemble create
}

namespace eval Footprint {
  proc definition {footprint_data} {
    ICeWall set_footprint $footprint_data
  }
  
  proc library {library_data} {
    ICeWall set_library $library_data
  }
    
  namespace export definition library
  namespace ensemble create
}

package provide ICeWall 0.1.0
