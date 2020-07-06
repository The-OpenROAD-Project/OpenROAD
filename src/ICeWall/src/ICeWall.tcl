namespace eval ICeWall {
  variable cells {}
  variable db
  variable default_orientation {bottom R0 right R90 top R180 left R270 ll R0 lr MY ur R180 ul MX}
  variable connect_pins_by_abutment
  variable idx {fill 0}

# Messages:
# 
# Information
#
# Warning
# 
# Error
# 4  "Cannot find a terminal $name to associate with bondpad bp_${name}"
# 11 "Expected instance $name for signal $name not found"
# 12 "Cannot find pin $pin_name of [$inst getName] ([[$inst getMaster] getName])"
# 16 "Cannot find pin $pin_name on $inst_name ([[$inst getMaster] getName])"
# 
# Critical
# 1  "Incorrect signal assignments ([llength $errors]) found"
# 2  "Cannot create instance for $padcell"
# 3  "Cannot create instance for $cell_name"
# 5  "Illegal orientation $orient specified"
# 6  "Illegal orientation $orient specified"
# 7  "File $signal_assignment_file not found"
# 8  "Cannot find cell $name in the database"
# 9  "Expected 1, 2 or 4 arguments, got [llength $args]"
# 10 "Expected 1, 2 or 4 arguments, got [llength $args]"
# 13 "Expected instance $name for signal $name not found"
# 14 "Net ${signal}_$section already exists, so cannot be used in the pad ring"
# 15 "Cannot find breaker cell $inst_name"
# 17 "Cannot find pin $pin_name (abutment signal=$pin_name) on $inst_name ([[$inst getMaster] getName])"
#

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

  proc information {id message} {
    puts [set_message INFO [format "\[ICEW-%04d\] %s" $id $message]]
  }

  proc warning {id message} {
    puts [set_message WARN [format "\[ICEW-%04d\] %s" $id $message]]
  }

  proc err {id message} {
    puts [set_message ERROR [format "\[ICEW-%04d\] %s" $id $message]]
  }

  proc critical {id message} {
    error [set_message CRIT [format "\[ICEW-%04d\] %s" $id $message]]
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
        default {critical 6 "Illegal orientation $orient specified"}
      }

      return [list x $x y $y]
  }

  proc get_orient {padcell {element "cell"}} {
    variable footprint
    variable library
    
    set side_name [get_side_name $padcell]
    if {[dict exists $footprint padcells $side_name $padcell $element orient]} {
      set orient [dict get $footprint padcells $side_name $padcell $element orient]
    } else {
      if {$element == "cell"} {
        set cell_type [dict get $library types [get_pad_type $padcell]]
      } else {
        set cell_type [dict get $library types $element]
      }
      # debug "cell_type $cell_type orient $side_name"
      set orient [dict get $library cells $cell_type orient $side_name]
    }
  }
  
  proc update_location_data {padcell} {
    variable def_units
    variable bondpad_width
    variable bondpad_height
    variable library
    variable footprint

    set side_name [get_side_name $padcell]
    if {$side_name == "none"} {
      set inst [dict get $footprint place $padcell]
      set cell_type [dict get $library types [dict get $inst type]]
      set orient [dict get $inst cell orient]
    } else {
      set inst [dict get $footprint padcells $side_name $padcell]
      set cell_type [dict get $library types [get_pad_type $padcell]]
      set orient [get_orient $padcell]
    }
    
    if {[dict exists $library cells $cell_type]} {
      set cell_name [dict get $library cells $cell_type cell_name $side_name]
    } else {
      set cell_name $cell_type
    }
    set cell [get_cell_master $cell_name]
    set cell_width [$cell getWidth]
    set cell_height [$cell getHeight]

    if {[dict exists $inst cell centre]} {
      set centre [list \
        x [expr round([dict get $inst cell centre x] * $def_units)] \
        y [expr round([dict get $inst cell centre y] * $def_units)] \
      ]

      dict set inst cell scaled_origin [get_origin $centre $cell_width $cell_height $orient]
      dict set inst cell scaled_centre $centre

    } elseif {[dict exists $inst cell origin]} {
      # Ensure that we're working with database units
      set origin [list \
        x [expr round([dict get $inst cell origin x] * $def_units)] \
        y [expr round([dict get $inst cell origin y] * $def_units)] \
      ]

      dict set inst cell scaled_origin $origin
      dict set inst cell scaled_centre [get_centre $origin $cell_width $cell_height $orient]
    } else {
      # debug "No generation of scaled origin for $padcell"
    }

    # Presence of bondpad is optional
    if {[dict exists $inst bondpad]} {
      if {[dict exists $inst bondpad centre]} {
        set centre [list \
          x [expr round([dict get $inst bondpad centre x] * $def_units)] \
          y [expr round([dict get $inst bondpad centre y] * $def_units)] \
        ]

        dict set inst bondpad scaled_origin [get_origin $centre $bondpad_width $bondpad_height [get_orient $padcell bondpad]]
        dict set inst bondpad scaled_centre $centre
      } elseif {[dict exists $inst bondpad origin]} {
        set origin [list \
          x [expr round([dict get $inst bondpad origin x] * $def_units)] \
          y [expr round[[dict get $inst bondpad origin y] * $def_units)] \
        ]
        
        dict set inst bondpad scaled_origin $origin
        dict set inst bondpad scaled_origin [get_centre $origin $bondpad_width $bondpad_height [get_orient $padcell bondpad]]
      }
    }

    return $inst
  }
  
  proc normalize_locations {} {
    variable block
    variable footprint
    variable library
    variable db
    variable bondpad_width
    variable bondpad_height

    set units [$block getDefUnits]
    if {[dict exists $library types bondpad]} {
      set bondpad_cell [get_cell "bondpad" "top"]
      set bondpad_width [$bondpad_cell getWidth]
      set bondpad_height [$bondpad_cell getHeight]
    }

    foreach side_name {"bottom" "right" "top" "left"} {
      foreach padcell [dict keys [dict get $footprint padcells $side_name]] {
	dict set footprint padcells $side_name $padcell [update_location_data $padcell]
      }
    }

    foreach inst_name [dict keys [dict get $footprint place]] {
      dict set footprint place $inst_name [update_location_data $inst_name]
    }
  }

  proc order_padcells {} {
    variable footprint
    variable block
    
    foreach side_name {bottom right top left} { 
      set unordered_keys {}

      foreach key [dict keys [dict get $footprint padcells $side_name]] {
        if {![dict exists $footprint padcells $side_name $key cell scaled_origin]} {
          # debug "No scaled_origin calculated for $key"
        }
        if {$side_name == "top" || $side_name == "bottom"} {
          lappend unordered_keys [list $key [dict get $footprint padcells $side_name $key cell scaled_origin x]]
        } else {
          lappend unordered_keys [list $key [dict get $footprint padcells $side_name $key cell scaled_origin y]]
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

      dict set footprint padcells order $side_name $order

      foreach padcell [dict keys [dict get $footprint padcells $side_name]] {
        # Ensure instance exists in the design
        set name [get_inst_name $padcell]
        set type [get_pad_type $padcell]
        set cell [get_cell $type $side_name]
        if {[set inst [$block findInst $name]] == "NULL"} {
          if {[is_physical_only $type]} { 
            set inst [odb::dbInst_create $block $cell $name]
          } elseif {[dict exists $footprint create_padcells] && [dict get $footprint create_padcells]} {
            set inst [odb::dbInst_create $block $cell $name]
          } else {
            err 11 "Expected instance $name for signal [dict get $footprint padcells $side_name $padcell name] not found"
            continue
          }
        }
      }
    }

    # Keep full order
    dict set footprint full_order [concat \
      [dict get $footprint padcells order bottom] \
      [dict get $footprint padcells order right] \
      [dict get $footprint padcells order top] \
      [dict get $footprint padcells order left] \
    ]
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
  
  proc get_side_name {padcell} {
    variable footprint
    
    if {[dict exists $footprint side $padcell]} {
      return [dict get $footprint side $padcell]
    }
    # debug "Side none for $padcell"
    return "none"
  }
  
  proc get_pad_type {padcell} {
    variable footprint
    
    return [dict get $footprint padcells [get_side_name $padcell] $padcell type]
  }
  
  proc get_inst_name {padcell} {
    variable footprint

    if {[is_power_net [get_assigned_name $padcell]] || [is_ground_net [get_assigned_name $padcell]]} {
      return "u_$padcell"
    }
    set info [dict get $footprint padcells [get_side_name $padcell] $padcell]
    if {[dict exists $info pad_inst_name]} {
      return [format [dict get $info pad_inst_name] [get_name $padcell $info]]
    }
    
    if {[dict exists $footprint pad_inst_name]} {
      return [format [dict get $footprint pad_inst_name] [get_name $padcell $info]]
    }
    
    return "u_[get_name $padcell $info]"
  }
  
  proc get_pin_name {padcell {info {}}} {
    variable footprint
    
    set info [dict get $footprint padcells [get_side_name $padcell] $padcell]
    if {[dict exists $info pad_pin_name]} {
      return [format [dict get $info pad_pin_name] [get_name $padcell $info]]
    }

    if {[dict exists $footprint pad_pin_name]} {
      return [format [dict get $footprint pad_pin_name] [get_name $padcell $info]]
    }
    
    return "u_[get_name $padcell $info]"
  }
  
  proc get_assigned_name {padcell} {
    variable footprint 

    set info [dict get $footprint padcells [get_side_name $padcell] $padcell]
    return "[get_name $padcell $info]"
  }

  proc get_name {padcell info} {
    if {[dict exists $info name]} {
      return [dict get $info name]
    }
    return "$padcell"
  }
  
  proc assign_signals {signal_assignment_file} {
    variable footprint
    
    if {![file exists $signal_assignment_file]} {
      critical 7 "File $signal_assignment_file not found"
    }
    set errors {}
    set ch [open $signal_assignment_file]
    while {![eof $ch]} {
      set line [gets $ch]
      set line [regsub {\#.} $line {}]
      if {[llength $line] == 0} {continue}
      
      set pad_name [lindex $line 0]
      set signal_name [lindex $line 1]
      
      set found 0
      foreach side "bottom right top left" {
        if {[dict exists $footprint padcells $side $pad_name]} {
          # debug "Assigning $signal_name to $pad_name"
          dict set footprint padcells $side $pad_name name "$signal_name"
          dict for {key value} [lrange $line 2 end] {
            dict set footprint padcells $side $pad_name $key $value
          }
          set found 1
          break
        }
      }
      
      if {$found == 0} {
        lappend errors "Pad id $pad_name not found in footprint"
      }  
    }
    
    if {[llength $errors] > 0} {
      set str "\n"
      foreach msg $errors {
         set str "$str\n  $msg"
      }
      critical 1 "$str\nIncorrect signal assignments ([llength $errors]) found"
    }
    
    close $ch
  }

  proc load_footprint {footprint_file} {
    variable footprint
    variable db
    variable tech
    variable block
    variable def_units
    variable corner_width
    variable chip_width 
    variable chip_height

    source $footprint_file
    
    set db [::ord::get_db]
    set tech [$db getTech]
    set block [[$db getChip] getBlock]

    set def_units [$tech getDbUnitsPerMicron]

    set corner_width [[get_cell corner ll] getWidth]

    set chip_width  [expr round(([lindex [dict get $footprint die_area] 2] - [lindex [dict get $footprint die_area] 0]) * $def_units)]
    set chip_height [expr round(([lindex [dict get $footprint die_area] 3] - [lindex [dict get $footprint die_area] 1]) * $def_units)]
    set_offsets     [dict get $footprint offsets]

    # Allow us to lookup which side a padcell is placed on.
    foreach side_name {bottom right top left} { 
      foreach padcell [dict keys [dict get $footprint padcells $side_name]] {
        dict set footprint side $padcell $side_name
      }
    }
  }
  
  proc get_tracks {} {
    variable footprint 

    if {![dict exists $footprint tracks]} {
      dict set floorplan tracks "tracks.info"
    }

    if {![file exists [dict get $footprint tracks]]} {
      write_track_file [dict get $footprint tracks] [dict get $footprint core_area]
    }
    
    return [dict get $footprint tracks]
  }

  proc get_abutment_nets {padcell {idx 0}} {
    variable footprint
    variable library
    variable block
    
    set abutment_nets {}
    if {[dict exists $library connect_by_abutment]} {
      set breakers {}
      if {[dict exists $library breakers]} {
        set breakers [dict get $library breakers]
      }
      set side_name [dict get $footprint side $padcell]
      set type [get_pad_type $padcell]
      set cell_type [dict get $library types $type]
      set inst_name [get_inst_name $padcell]
      set inst [$block findInst $inst_name]

      foreach pin [dict get $library connect_by_abutment] {
        set pin_name $pin
        if {[lsearch $breakers $type] > -1 && [dict exists $library cells $cell_type breaks $pin_name]} {
          # Breaker cells have a left / right version of the pin_name in the library metadata
          set pin_name [lindex [dict get $library cells $cell_type breaks $pin_name] $idx]
        }
        if {[dict exists $library cells $cell_type connect $pin]} {
          set pin_name [dict get $library cells $cell_type connect $pin]
        }
        set iterm [$inst findITerm $pin_name]
        if {$iterm == "NULL"} {
          critical 17 "Cannot find pin $pin_name (abutment signal=$pin_name) on $inst_name ([[$inst getMaster] getName])"
        } 
        if {[set net [$iterm getNet]] != "NULL"} {
          dict set abutment_nets $pin $net
        } else {
          err 16 "Cannot find net for pin $pin_name (abutment signal=$pin_name) on $inst_name ([[$inst getMaster] getName])"
        }
      }      
    }
    
    return $abutment_nets
  }

  proc connect_abutment_nets {inst abutment_nets} {
    variable library
    
    if {[dict exists $library connect_by_abutment]} {
      dict for {pin_name net} $abutment_nets {
        set mterm [[$inst getMaster] findMTerm $pin_name]
        if {$mterm == "NULL"} {
          err 12 "Cannot find pin $pin_name of [$inst getName] ([[$inst getMaster] getName])"
          continue
        }
        set iterm [odb::dbITerm_connect $inst $net $mterm]
        $iterm setSpecial
      }
    }
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

  proc fill_between_padcells {} {
    variable block
    variable tech
    variable chip_width 
    variable chip_height
    variable edge_bottom_offset 
    variable edge_right_offset 
    variable edge_top_offset 
    variable edge_left_offset
    variable footprint
    variable corner_width
    
    foreach side_name {bottom right top left} {
      switch $side_name \
        "bottom" {
          set fill_start [expr $edge_left_offset + $corner_width]
          set fill_end   [expr $chip_width - $edge_right_offset - $corner_width]
        } \
        "right"  {
          set fill_start [expr $edge_bottom_offset + $corner_width]
          set fill_end   [expr $chip_height - $edge_top_offset - $corner_width]
        } \
        "top"    {
          set fill_start [expr $chip_width - $edge_right_offset - $corner_width]
          set fill_end   [expr $edge_left_offset + $corner_width]
        } \
        "left"   {
          set fill_start [expr $chip_height - $edge_top_offset - $corner_width]
          set fill_end   [expr $edge_bottom_offset + $corner_width]
        }

      foreach padcell [dict get $footprint padcells order $side_name] {
        set name [get_inst_name $padcell]
        set type [get_pad_type $padcell]
        set cell [get_cell $type $side_name]

        set cell_height [expr max([$cell getHeight],[$cell getWidth])]
        set cell_width  [expr min([$cell getHeight],[$cell getWidth])]

        if {[set inst [$block findInst $name]] == "NULL"} {
          # err 13 "Expected instance $name for signal $name not found"
          continue
        }

        set x [dict get $footprint padcells $side_name $padcell cell scaled_origin x]
        set y [dict get $footprint padcells $side_name $padcell cell scaled_origin y]

        $inst setOrigin $x $y
        $inst setOrient [get_orient $padcell]
        $inst setPlacementStatus "FIRM"
        set bbox [$inst getBBox]

        switch $side_name \
          "bottom" {
            # debug "cell_width: $cell_width cell_height: $cell_height"
            fill_box $fill_start [$bbox yMin] [$bbox xMin] [$bbox yMax] $side_name [get_abutment_nets $padcell]
            set fill_start [$bbox xMax]
          } \
          "right"  {
            # debug "fill after [$inst getName]"
            fill_box [$bbox xMin] $fill_start [$bbox xMax] [$bbox yMin] $side_name [get_abutment_nets $padcell]
            set fill_start [$bbox yMax]
          } \
          "top" {
            fill_box [$bbox xMax] [$bbox yMin] $fill_start [$bbox yMax] $side_name [get_abutment_nets $padcell]
            # debug "added_cell: [$inst getName] ($x $y) [$cell getName] [$cell getWidth] x [$cell getHeight]"
            set fill_start [$bbox xMin]
            # debug "$side_name: fill_start = $fill_start"
          } \
          "left" {
            fill_box [$bbox xMin] [$bbox yMax] [$bbox xMax] $fill_start $side_name [get_abutment_nets $padcell]
            set fill_start [$bbox yMin]
          }


        dict set pad_ring $name $inst
        dict incr idx $type
      } 

      # debug "$side_name: fill_start = $fill_start"
      # debug "$side_name: fill_end   = $fill_end"
      switch $side_name \
        "bottom" {
          fill_box $fill_start [$bbox yMin] $fill_end [$bbox yMax] $side_name [get_abutment_nets $padcell 1]
        } \
        "right"  {
          fill_box [$bbox xMin] $fill_start [$bbox xMax] $fill_end $side_name [get_abutment_nets $padcell 1]
        } \
        "top" {
          fill_box $fill_end [$bbox yMin] $fill_start [$bbox yMax] $side_name [get_abutment_nets $padcell 1]
        } \
        "left" {
          fill_box [$bbox xMin] $fill_end [$bbox xMax] $fill_start $side_name [get_abutment_nets $padcell 1]
        }

      foreach padcell [dict get $footprint padcells order $side_name] {
        set signal_name [get_inst_name $padcell]
        set side_name [get_side_name $padcell]
        set type [get_pad_type $padcell]

        if {[dict exists $footprint padcells $side_name $padcell bondpad]} {
          # Padcells have separate bondpads that need to be added to the design
          set x [dict get $footprint padcells $side_name $padcell bondpad scaled_origin x]
          set y [dict get $footprint padcells $side_name $padcell bondpad scaled_origin y]
          set cell [get_cell bondpad $side_name]

          set inst [odb::dbInst_create $block $cell "bp_${padcell}"]
          $inst setOrigin $x $y
          $inst setOrient [get_orient $padcell bondpad]
          $inst setPlacementStatus "FIRM"

          set term [$block findBTerm [get_pin_name $padcell]]
          if {$term != "NULL"} {
            set net [$term getNet]
            foreach iterm [$net getITerms] {
              $iterm setSpecial
            }
            $net setSpecial

            set pin [odb::dbBPin_create $term]
            set layer [$tech findLayer [dict get $footprint pin_layer]]
            set x [dict get $footprint padcells $side_name $padcell bondpad scaled_centre x]
            set y [dict get $footprint padcells $side_name $padcell bondpad scaled_centre y]
            odb::dbBox_create $pin $layer [expr $x - [$layer getWidth] / 2] [expr $y - [$layer getWidth] / 2] [expr $x + [$layer getWidth] / 2] [expr $y + [$layer getWidth] / 2]
            $pin setPlacementStatus "FIRM"
          } else {
            if {$type == "sig"} {
              err 4 "Cannot find a terminal [get_pin_name $padcell] for ${signal_name} to associate with bondpad bp_${signal_name}"
            } else {
              set assigned_name [get_assigned_name $padcell]
              if {[is_power_net $assigned_name]} {
                set type "POWER"
              } elseif {[is_ground_net $assigned_name]} {
                set type "GROUND"
              } else {
                set type "SIGNAL"
              }
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
                # $term setSupplyPin $type
              }
              $net setSpecial
              $net setSigType $type

              set pin [odb::dbBPin_create $term]
              set layer [$tech findLayer [dict get $footprint pin_layer]]
              set x [dict get $footprint padcells $side_name $padcell bondpad scaled_centre x]
              set y [dict get $footprint padcells $side_name $padcell bondpad scaled_centre y]
              odb::dbBox_create $pin $layer [expr $x - [$layer getWidth] / 2] [expr $y - [$layer getWidth] / 2] [expr $x + [$layer getWidth] / 2] [expr $y + [$layer getWidth] / 2]
              $pin setPlacementStatus "FIRM"
            }
          }
        }
      }
    }
    dict set pad_ring corner_ll [set inst [odb::dbInst_create $block [set corner [get_cell corner ll]] "CORNER_LL"]]
    $inst setOrigin $edge_left_offset $edge_bottom_offset
    $inst setOrient [get_cell_orientation [$corner getName] ll]

    $inst setPlacementStatus "FIRM"
    connect_abutment_nets $inst [get_abutment_nets [lindex [dict get $footprint padcells order "bottom"] 0]]

    dict set pad_ring corner_lr [set inst [odb::dbInst_create $block [set corner [get_cell corner lr]] "CORNER_LR"]]
    $inst setOrigin [expr ($chip_width - $edge_right_offset)] $edge_bottom_offset
    $inst setOrient "R90"
    $inst setPlacementStatus "FIRM"
    connect_abutment_nets $inst [get_abutment_nets [lindex [dict get $footprint padcells order "right"] 0]]

    dict set pad_ring corner_ur [set inst [odb::dbInst_create $block [set corner [get_cell corner ur]] "CORNER_UR"]]
    $inst setOrigin [expr ($chip_width - $edge_right_offset)] [expr ($chip_height - $edge_top_offset)]
    $inst setOrient "R180"
    $inst setPlacementStatus "FIRM"
    connect_abutment_nets $inst [get_abutment_nets [lindex [dict get $footprint padcells order "top"] 0]]

    dict set pad_ring corner_ul [set inst [odb::dbInst_create $block [set corner [get_cell corner ul]] "CORNER_UL"]]
    $inst setOrigin $edge_left_offset [expr ($chip_height - $edge_top_offset)]
    $inst setOrient "R270"
    $inst setPlacementStatus "FIRM"
    connect_abutment_nets $inst [get_abutment_nets [lindex [dict get $footprint padcells order "left"] 0]]
  }  
  
  proc place_additional_cells {} {
    variable block
    variable footprint

    dict for {cell_name inst_info} [dict get $footprint place] {
      set name [dict get $inst_info name]
      set type [dict get $inst_info type]
      set cell [get_cell $type]

      if {[set inst [$block findInst $name]] == "NULL"} {
        set inst [odb::dbInst_create $block $cell $name]
      }

      if {$inst == "NULL"} {
        set str "init_footprint: inst_info $inst_info"
        set str "$str\ninit_footprint: name $name"
        set str "$str\ninit_footprint: type $type"
        set str "$str\ninit_footprint: cell $cell"
        critical 3 "$str\nCannot create instance for $cell_name"
      }

      set x [dict get $inst_info cell scaled_origin x]
      set y [dict get $inst_info cell scaled_origin y]

      $inst setOrigin $x $y
      $inst setOrient [dict get $inst_info cell orient]
      $inst setPlacementStatus "FIRM"
    }
  }

  proc connect_by_abutment {} {
    variable footprint
    variable library
    variable block

    if {[dict exists $library connect_by_abutment]} {
      # Breaker cells affect the connectivity of signals connected by abutment.
      set segment {}
      array set pad_segment {}
      set breaker_types [dict get $library breakers]

      # Start each signal index at 0, and increment each time a breaker breaks the abutted signal
      foreach signal [dict get $library connect_by_abutment] {
        dict set segment $signal cur_index 0
        set pad_segment($signal,0) {}
      }

      foreach padcell [dict get $footprint full_order] {
        set side_name [dict get $footprint side $padcell]

        set name [get_inst_name $padcell]
        set type [get_pad_type $padcell]

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
              lappend pad_segment($signal,[dict get $segment $signal cur_index]) [list inst_name $name pin_name $pin_name]
            }
          }
        } else {
          foreach signal [dict get $library connect_by_abutment] {
            set cell [dict get $library types $type]
            set pin_name $signal
            if {[dict exists $library cells $cell connect $signal]} {
              set pin_name [dict get $library cells $cell connect $signal]
            }
            lappend pad_segment($signal,[dict get $segment $signal cur_index]) [list inst_name $name pin_name $pin_name]
          }
        }
      }
# debug [array get pad_segment]      
      foreach item [array names pad_segment] {
        regexp {([^,]*),(.*)} $item - signal idx
        dict set segment cells $signal $idx $pad_segment($item)
      }
      dict for {signal seg} [dict get $segment cells] {
        set indexes [lsort -integer [dict keys $seg]]
        set first [lindex $indexes 0]
        set last [lindex $indexes end]
        
        dict set seg $first [concat [dict get $seg $first] [dict get $seg $last]]
        dict set segment cells $signal [dict remove $seg $last]
      }

      # Wire up the nets that connect by abutment
      # Need to set these nets as SPECIAL so the detail router does not try to route them.
      dict for {signal sections} [dict get $segment cells] {
        foreach section [dict keys $sections] {
          if {[set net [$block findNet "${signal}_$section"]] != "NULL"} {
            # critical 14 "Net ${signal}_$section already exists, so cannot be used in the pad ring"
          } else {
            set net [odb::dbNet_create $block "${signal}_$section"]
          }
          $net setSpecial
          foreach inst_pin_name [dict get $sections $section] {
            set inst_name [dict get $inst_pin_name inst_name]
            set pin_name [dict get $inst_pin_name pin_name]

            if {[set inst [$block findInst $inst_name]] == "NULL"} {
              continue
            }
            
            set mterm [[$inst getMaster] findMTerm $pin_name]
            if {$mterm != "NULL"} {
              set iterm [odb::dbITerm_connect $inst $net $mterm]
              $iterm setSpecial
            } else {
              err 72 "No term $signal found on $inst_name"
            }
          }
        }
      }

      dict for {signal breakers} [dict get $segment breaker] {
        foreach section [dict keys $breakers] {
          set inst_name [dict keys [dict get $breakers $section]]
          # debug "signal: $signal, section: $section, inst: $inst_name"
          if {[set inst [$block findInst $inst_name]] == "NULL"} {
            critical 15 "Cannot find breaker cell $inst_name"
          }
          
          if {[set before_net [$block findNet "${signal}_$section"]] == "NULL"} { 
            set before_net [odb::dbNet_create $block "${signal}_$section"]
            $before_net setSpecial
          }
          set before_pin [[$inst getMaster] findMTerm [dict get $breakers $section $inst_name before_pin]]
          if {$before_pin == "NULL"} {
            # debug "Cannot find pin [dict get  $breakers $section $inst_name before_pin] on $inst_name ([[$inst getMaster] getName])"
          } else {
            set iterm [odb::dbITerm_connect $inst $before_net $before_pin]
            $iterm setSpecial
          }
          
          # Loop back to zero if we run out of sections
          set section_id [expr $section + 1]
          if {![dict exists $segment cells $signal $section_id]} {
            set section_id 0
          }
          if {[set after_net [$block findNet "${signal}_$section_id"]] == "NULL"} { 
            set after_net [odb::dbNet_create $block "${signal}_$section_id"]
            $after_net setSpecial
          }
          set after_pin  [[$inst getMaster] findMTerm [dict get $breakers $section $inst_name after_pin]]
          if {$after_pin == "NULL"} {
            # debug "Cannot find pin [dict get  $breakers $section $inst_name after_pin] on $inst_name ([[$inst getMaster] getName])"
          } else {
            set iterm [odb::dbITerm_connect $inst $after_net  $after_pin]
            $iterm setSpecial
          }
        }
      }
    }
  }
  
  proc global_assignments {} {
    variable footprint
    variable library

    # Determine actual parameter values for each pad instance
    # Use separate namespaces to ensure there is no clash in evaluation
    if {[namespace exists pad_inst]} {
      namespace delete pad_inst
    }
    namespace eval pad_inst {}
    # Evaluate all the parameters for all padcell instances
    foreach padcell [dict get $footprint full_order] {
      set side [dict get $footprint side $padcell]
      # debug "padcell - [dict get $footprint padcells $side $padcell]"
      set type [dict get $footprint padcells $side $padcell type]
      set library_element [dict get $library types $type]
      set library_cell [dict get $library cells $library_element]

      namespace eval "pad_inst_values::$padcell" {}
      if {$type == "sig"} {
        set "pad_inst_values::${padcell}::signal" [dict get $footprint padcells $side $padcell name]
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
  
  proc init_footprint {{signal_assignment_file ""}} {
    # Perform signal assignment 
    if {$signal_assignment_file != ""} {
      assign_signals $signal_assignment_file
    }

    normalize_locations
    order_padcells
    connect_by_abutment
    fill_between_padcells
    global_assignments  

    # Place miscellaneous other cells
    place_additional_cells
  }
  
  proc get_cell_master {name} {
    variable db

    if {[set cell [$db findMaster $name]] != "NULL"} {return $cell}
    
    critical 8 "Cannot find cell $name in the database"
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

  proc get_cell_orientation {name position} {
    variable library 
    
    return [dict get $library cells $name orient $position]
  }

  proc get_cell {type {side "none"}} {
    variable library 

    set type_name [dict get $library types $type]
    
    if {[dict exists $library cells $type_name cell_name]} {
      return [get_cell_master [dict get $library cells $type_name cell_name $side]]
    } else {
      return [get_cell_master $type_name]
    }
  }

  proc is_physical_only {type} {
    variable library 
    
    set type_name [dict get $library types $type]
    
    if {[dict exists $library cells $type_name physical_only]} {
      return [dict get $library cells $type_name physical_only]
    } 
    
    return 0
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

  proc set_pad_order {cell_type_order sides} {
    variable order
    foreach side $sides {
      dict set order $side $cell_type_order
    }
  }
  
  proc set_block {design_block} {
    variable db
    variable block
    variable chip_width 
    variable chip_height
    
    set db [$design_block getDataBase]
    set block $design_block
    
    set chip_width   [[$block getDieArea] xMax]
    set chip_height  [[$block getDieArea] yMax]
  }
  
  proc set_offsets {args} {
    variable db
    variable edge_bottom_offset 
    variable edge_right_offset 
    variable edge_top_offset 
    variable edge_left_offset
    variable def_units
    
    if {[llength $args] == 1} {
      set edge_bottom_offset [expr $args * $def_units]
      set edge_right_offset  [expr $args * $def_units]
      set edge_top_offset    [expr $args * $def_units]
      set edge_left_offset   [expr $args * $def_units]
    } elseif {[llength $args] == 2} {
      set edge_bottom_offset [expr [lindex $args 0] * $def_units]
      set edge_right_offset  [expr [lindex $args 1] * $def_units]
      set edge_top_offset    [expr [lindex $args 0] * $def_units]
      set edge_left_offset   [expr [lindex $args 1] * $def_units]
    } elseif {[llength $edge_offset] == 4} {
      set edge_bottom_offset [expr [lindex $args 0] * $def_units]
      set edge_right_offset  [expr [lindex $args 1] * $def_units]
      set edge_top_offset    [expr [lindex $args 2] * $def_units]
      set edge_left_offset   [expr [lindex $args 3] * $def_units]
    } else {
      critical 9 "Expected 1, 2 or 4 arguments, got [llength $args]"
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
      critical 10 "Expected 1, 2 or 4 arguments, got [llength $args]"
    }
  }
  
  proc get_core_area {} {
    variable footprint
    
    variable chip_width
    variable chip_height
    variable edge_bottom_offset
    variable edge_right_offset
    variable edge_top_offset
    variable edge_left_offset
    variable corner_width
    variable inner_bottom_offset
    variable inner_right_offset
    variable inner_top_offset
    variable inner_left_offset
    
    if {[dict exists $footprint core_area]} {
      return [dict get $footprint core_area]
    }
    
    return [list \
      [expr $edge_left_offset + $corner_width + $inner_left_offset] \
      [expr $edge_bottom_offset + $corner_width + $inner_bottom_offset] \
      [expr $chip_width - $edge_right_offset - $corner_width - $inner_right_offset] \
      [expr $chip_height - $edge_top_offset - $corner_width - $inner_top_offset] \
    ]
  }

  proc init_index {} {
    variable cells
    variable idx
    
    foreach key [dict keys $cells] {
      dict set idx $key 0
    }
  }

  proc fill_box {xmin ymin xmax ymax side abutment_nets} {
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
      set orient [get_cell_orientation $spacer_type $side]
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
      $inst setOrigin $x $y
      $inst setOrient $orient
      $inst setPlacementStatus "FIRM"

      connect_abutment_nets $inst $abutment_nets

      dict incr idx $type
    }
  }

  proc get_die_area {} {
    variable footprint
    
    return [dict get $footprint die_area]
  }
  namespace export set_inner_offset
  namespace export add_cell 
  namespace export set_pad_order set_offsets set_block
  namespace export init_footprint set_cell_orientation load_footprint

  namespace export get_die_area get_core_area get_tracks set_footprint set_library
  
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
