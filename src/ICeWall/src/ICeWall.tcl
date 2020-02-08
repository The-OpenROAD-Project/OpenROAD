namespace eval ICeWall {
  variable cells {}
  variable db
  variable default_orientation {bottom R0 right R90 top R180 left R270 ll R0 lr MY ur R180 ul MX}
  variable connect_pins_by_abutment
  variable idx {fill 0}

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
        default {error "Illegal orientation $orient specified"}
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
        default {error "Illegal orientation $orient specified"}
      }

      return [list x $x y $y]
  }

  proc update_location_data {inst {side_name ""}} {
    variable def_units
    variable bondpad_width
    variable bondpad_height
    variable library
    
    set cell_type [dict get $library types [dict get $inst type]]
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

      dict set inst cell scaled_origin [get_origin $centre $cell_width $cell_height [dict get $inst cell orient]]
      dict set inst cell scaled_centre $centre

    } elseif {[dict exists $inst cell origin]} {
      # Ensure that we're working with database units
      set origin [list \
        set x [expr round([dict get $inst cell origin x] * $def_units)]
        set y [expr round[[dict get $inst cell origin y] * $def_units)]
      ]

      dict set inst cell scaled_origin $origin
      dict set inst cell scaled_centre [get_centre $origin $cell_width $cell_height [dict get $inst cell orient]]
    }

    # Presence of bondpad is optional
    if {[dict exists $inst bondpad centre]} {
      set bondpad [dict exists $library cells $cell_type]
      set centre [list \
        x [expr round([dict get $inst bondpad centre x] * $def_units)] \
        y [expr round([dict get $inst bondpad centre y] * $def_units)] \
      ]
      dict set inst bondpad scaled_origin [get_origin $centre $bondpad_width $bondpad_height [dict get $inst bondpad orient]]
      dict set inst bondpad scaled_centre $centre
    } elseif {[dict exists $inst bondpad origin]} {
      set x [expr round([dict get $inst bondpad origin x] * $def_units)]
      set y [expr round[[dict get $inst bondpad origin y] * $def_units)]

      dict set inst bondpad scaled_origin $origin
      dict set inst bondpad scaled_origin [get_centre $origin $bondpad_width $bondpad_height [dict get $inst cell orient]]
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
      set side [dict get $footprint padcells $side_name]

      dict for {inst_name inst} $side {
	dict set footprint padcells $side_name $inst_name [update_location_data $inst $side_name]
      }
    }

    dict for {inst_name inst} [dict get $footprint place] {
      dict set footprint place $inst_name [update_location_data $inst]
    }
  }

  proc order_padcells {} {
    variable footprint

    foreach side_name "bottom right top left" { 
      set unordered_keys {}

      foreach key [dict keys [dict get $footprint padcells $side_name]] {
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
  
  proc init_fill {side_name} {
    variable fill_start
    variable fill_end

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
  }

  proc get_inst_name {inst_name {inst {}}} {
    variable footprint
    
    if {[dict exists $inst pad_inst_name]} {
      return [format [dict get $inst pad_inst_name] [get_name $inst_name $inst]]
    }
    
    if {[dict exists $footprint pad_inst_name]} {
      return [format [dict get $footprint pad_inst_name] [get_name $inst_name $inst]]
    }
    
    return "u_[get_name $inst_name $inst]"
  }
  
  proc get_name {inst_name inst} {
    if {[dict exists $inst name]} {
      return [dict get $inst name]
    }
    return "$inst_name"
  }
  
  proc assign_signals {signal_assignment_file} {
    variable footprint
    
    if {![file exists $signal_assignment_file]} {
      error "File $signal_assignment_file not found"
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
          # puts "assign_signals: Assigning $signal_name to $pad_name"
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
      foreach msg $errors {
        puts "  $msg"
      }
      error "Incorrect signal assignments ([llength $errors]) found"
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
  
  proc init_footprint {{signal_assignment_file ""}} {
    variable db
    variable block
    variable tech
    variable chip_width 
    variable chip_height
    variable edge_bottom_offset 
    variable edge_right_offset 
    variable edge_top_offset 
    variable edge_left_offset
    variable footprint
    variable library
    variable corner_width

    # Perform signal assignment 
    if {$signal_assignment_file != ""} {
      assign_signals $signal_assignment_file
    }
    
    # Place IO padring and bondpads
    #set cells [dict get $library cells]

    normalize_locations
    order_padcells
    
    foreach side_name {bottom right top left} {
      set side [dict get $footprint padcells $side_name]
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
        set inst_info [dict get $footprint padcells $side_name $padcell]
        set name [get_inst_name $padcell $inst_info]
        set type [dict get $inst_info type]
        set cell [get_cell $type $side_name]
        
        if {[set inst [$block findInst $name]] == "NULL"} {
          set inst [odb::dbInst_create $block $cell $name]
        }

        if {$inst == "NULL"} {
          puts "init_footprint: inst_info $inst_info"
          puts "init_footprint: name $name"
          puts "init_footprint: type $type"
          puts "init_footprint: cell $cell"
          error "Cannot create instance for $padcell"
        }

        set x [dict get $inst_info cell scaled_origin x]
        set y [dict get $inst_info cell scaled_origin y]
        
        $inst setOrigin $x $y
        $inst setOrient [dict get $inst_info cell orient]
        $inst setPlacementStatus "FIRM"

        switch $side_name \
          "bottom" {
            fill_box $fill_start $y $x [expr $y + [$cell getHeight]] $side_name
            set fill_start [expr $x + [$cell getWidth]]
          } \
          "right"  {
            fill_box [expr $x - [$cell getHeight]] $fill_start $x $y $side_name
            set fill_start [expr $y + [$cell getWidth]]
          } \
          "top" {
            fill_box $x [expr $y - [$cell getHeight]] $fill_start $y $side_name
            set fill_start [expr $x - [$cell getWidth]]
          } \
          "left" {
            fill_box $x $y [expr $x + [$cell getHeight]] $fill_start $side_name
            set fill_start [expr $y - [$cell getWidth]]
          }


        dict set pad_ring $name $inst
        dict incr idx $type
      }
      switch $side_name \
        "bottom" {
          fill_box $fill_start $y $fill_end [expr $y + [$cell getHeight]] $side_name
        } \
        "right"  {
          fill_box [expr $x - [$cell getHeight]] $fill_start $x $fill_end $side_name
        } \
        "top" {
          fill_box $fill_end [expr $y - [$cell getHeight]] $fill_start $y $side_name
        } \
        "left" {
          fill_box $x $fill_end [expr $x + [$cell getHeight]] $fill_start $side_name
        }

      foreach padcell [dict get $footprint padcells order $side_name] {
        set inst_info [dict get $footprint padcells $side_name $padcell]
        set name [get_name $padcell $inst_info]
        set type [dict get $inst_info type]
        
        if {[dict exists $inst_info bondpad]} {
          set orientation [dict get $inst_info bondpad orient]
          set x [dict get $inst_info bondpad scaled_origin x]
          set y [dict get $inst_info bondpad scaled_origin y]
          set cell [get_cell bondpad $side_name]
          set inst [odb::dbInst_create $block $cell "bp_${name}"]

          $inst setOrigin $x $y
          $inst setOrient [dict get $inst_info bondpad orient]
          $inst setPlacementStatus "FIRM"

          set term [$block findBTerm $name]
          if {$term != "NULL"} {
            set pin [odb::dbBPin_create $term]
            set layer [$tech findLayer [dict get $footprint pin_layer]]
            set x [dict get $inst_info bondpad scaled_centre x]
            set y [dict get $inst_info bondpad scaled_centre y]
            odb::dbBox_create $pin $layer [expr $x - [$layer getWidth] / 2] [expr $y - [$layer getWidth] / 2] [expr $x + [$layer getWidth] / 2] [expr $y + [$layer getWidth] / 2]
            $pin setPlacementStatus "FIRM"
          }
        }
      }
    }

    # Place miscellaneous other cells
    dict for {cell_name inst_info} [dict get $footprint place] {
      set name [get_inst_name $cell_name $inst_info]
      set type [dict get $inst_info type]
      set cell [get_cell $type $side_name]

      if {[set inst [$block findInst $name]] == "NULL"} {
        set inst [odb::dbInst_create $block $cell $name]
      }

      if {$inst == "NULL"} {
        puts "init_footprint: inst_info $inst_info"
        puts "init_footprint: name $name"
        puts "init_footprint: type $type"
        puts "init_footprint: cell $cell"
        error "Cannot create instance for $cell_name"
      }

      set x [dict get $inst_info cell scaled_origin x]
      set y [dict get $inst_info cell scaled_origin y]

      $inst setOrigin $x $y
      $inst setOrient [dict get $inst_info cell orient]
      $inst setPlacementStatus "FIRM"
    }

    dict set pad_ring corner_ll [set inst [odb::dbInst_create $block [set corner [get_cell corner ll]] "CORNER_LL"]]
    $inst setOrigin $edge_left_offset $edge_bottom_offset
    $inst setOrient [get_cell_orientation [$corner getName] ll]

    $inst setPlacementStatus "FIRM"

    dict set pad_ring corner_lr [set inst [odb::dbInst_create $block [set corner [get_cell corner lr]] "CORNER_LR"]]
    $inst setOrigin [expr ($chip_width - $edge_right_offset)] $edge_bottom_offset
    $inst setOrient "R90"
    $inst setPlacementStatus "FIRM"

    dict set pad_ring corner_ur [set inst [odb::dbInst_create $block [set corner [get_cell corner ur]] "CORNER_UR"]]
    $inst setOrigin [expr ($chip_width - $edge_right_offset)] [expr ($chip_height - $edge_top_offset)]
    $inst setOrient "R180"
    $inst setPlacementStatus "FIRM"

    dict set pad_ring corner_ul [set inst [odb::dbInst_create $block [set corner [get_cell corner ul]] "CORNER_UL"]]
    $inst setOrigin $edge_left_offset [expr ($chip_height - $edge_top_offset)]
    $inst setOrient "R270"
    $inst setPlacementStatus "FIRM"
  }
  
  proc set_connect_by_abutment {pin_names} {
    variable connect_pins_by_abutment
    set connect_pins_by_abutment $pin_names
  }
  
  proc connect_by_abutment {pin_names} {
    variable connect_pins_by_abutment
    foreach net_name $connect_pins_by_abutment {
      set net [$block findNet $net_name]
      if {$net == "NULL"} {
        set net [odb::dbNet_create $block $net_name]
      }
    }

    foreach inst [$block getInsts] {
      set master [$inst getMaster]
      foreach pin_name $connect_pins_by_abutment {
        set mterm [$master findMTerm $pin_name]
        odb::dbITerm_connect $inst $net $mterm
      }
    }
  }
  
  proc get_cell_master {name} {
    variable db

    if {[set cell [$db findMaster $name]] != "NULL"} {return $cell}
    
    error "Cannot find cell $name in the database"
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

  proc get_cell {type side} {
    variable library 

    set type_name [dict get $library types $type]
    
    if {[dict exists $library cells $type_name cell_name]} {
      return [get_cell_master [dict get $library cells $type_name cell_name $side]]
    } else {
      return [get_cell_master $type_name]
    }
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
      error "Expected 1, 2 or 4 arguments, got [llength $args]"
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
      error "Expected 1, 2 or 4 arguments, got [llength $args]"
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

      set name [get_inst_name "${type}_[dict get $idx $type]"]
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

      $inst setOrigin $x $y
      $inst setOrient $orient
      $inst setPlacementStatus "FIRM"

      dict incr idx $type
    }
  }

  proc place_padring {} {
    variable block
    variable idx
    variable order
    variable edge_bottom_offset 
    variable edge_right_offset 
    variable edge_top_offset 
    variable edge_left_offset
    variable chip_width 
    variable chip_height
    variable corner_width
    variable default_orientation

    init_index
    
    foreach side {"bottom" "right" "top" "left"} {
      set total_width 0
      foreach pad [dict get $order $side] {
        set total_width [expr $total_width + [[get_cell $pad $side] getWidth]]
      }

      switch $side \
        "bottom" {set available_length [expr $chip_width - $edge_left_offset - $edge_right_offset - (2 * $corner_width)]} \
        "top"    {set available_length [expr $chip_width - $edge_left_offset - $edge_right_offset - (2 * $corner_width)]} \
        "left"   {set available_length [expr $chip_height - $edge_bottom_offset - $edge_top_offset - (2 * $corner_width)]} \
        "bottom" {set available_length [expr $chip_height - $edge_bottom_offset - $edge_top_offset - (2 * $corner_width)]}
        
      set step   [expr ($available_length - $total_width) / [llength [dict get $order $side]]]
      set offset [expr $step / 2]

      init_fill $side
      
      switch $side \
        "bottom" {
          set fill_start [expr $edge_left_offset + $corner_width]
          set fill_end   [expr $chip_width - $edge_right_offset - $corner_width]
          set x          [expr $edge_left_offset + $corner_width + $offset]
          set y          [expr $edge_bottom_offset]
        } \
        "right"  {
          set fill_start [expr $edge_bottom_offset + $corner_width]
          set fill_end   [expr $chip_height - $edge_top_offset - $corner_width]
          set x          [expr $chip_width - $edge_right_offset]
          set y          [expr $edge_bottom_offset + $corner_width + $offset]
        } \
        "top"    {
          set fill_start [expr $chip_width - $edge_right_offset - $corner_width]
          set fill_end   [expr $edge_left_offset + $corner_width]
          set x          [expr $chip_width - $edge_right_offset - $corner_width - $offset]
          set y          [expr $chip_height - $edge_top_offset]
        } \
        "left"   {
          set fill_start [expr $chip_height - $edge_top_offset - $corner_width]
          set fill_end   [expr $edge_bottom_offset + $corner_width]
          set x          [expr $edge_left_offset]
          set y          [expr $chip_height - $edge_top_offset - $corner_width - $offset]
        }

      for {set i 0} {$i < [llength [dict get $order $side]]} {incr i} {
        set type [lindex [dict get $order $side] $i]
        set name [get_inst_name "${type}_[dict get $idx $type]"]
        set cell [get_cell $type $side]
        set inst [odb::dbInst_create $block $cell $name]

        $inst setOrigin $x $y
        $inst setOrient [get_cell_orientation [$cell getName] $side]
        $inst setPlacementStatus "FIRM"

        switch $side \
          "bottom" {
            fill_box $fill_start $y $x [expr $y + [$cell getHeight]] $side
            set fill_start [expr $x + [$cell getWidth]]

            set x [expr $x + [$cell getWidth] + $step]
          } \
          "right"  {
            fill_box [expr $x - [$cell getHeight]] $fill_start $x $y $side
            set fill_start [expr $y + [$cell getWidth]]

            set y [expr $y + [$cell getWidth] + $step]
          } \
          "top" {
            fill_box $x [expr $y - [$cell getHeight]] $fill_start $y $side
            set fill_start [expr $x - [$cell getWidth]]

            set x [expr $x - [$cell getWidth] - $step]
          } \
          "left" {
            fill_box $x $y [expr $x + [$cell getHeight]] $fill_start $side
            set fill_start [expr $y - [$cell getWidth]]

            set y [expr $y - [$cell getWidth] - $step]
          }


        dict set pad_ring $name $inst
        dict incr idx $type
      }
      switch $side \
        "bottom" {
          fill_box $fill_start $y $fill_end [expr $y + [$cell getHeight]] $side
        } \
        "right"  {
          fill_box [expr $x - [$cell getHeight]] $fill_start $x $fill_end $side
        } \
        "top" {
          fill_box $fill_end [expr $y - [$cell getHeight]] $fill_start $y $side
        } \
        "left" {
          fill_box $x $fill_end [expr $x + [$cell getHeight]] $fill_start $side
        }
    }

    dict set pad_ring corner_ll [set inst [odb::dbInst_create $block [set corner [get_cell corner ll]] "CORNER_LL"]]
    $inst setOrigin $edge_left_offset $edge_bottom_offset
    $inst setOrient [get_cell_orientation [$corner getName] ll]

    $inst setPlacementStatus "FIRM"

    dict set pad_ring corner_lr [set inst [odb::dbInst_create $block [set corner [get_cell corner lr]] "CORNER_LR"]]
    $inst setOrigin [expr ($chip_width - $edge_right_offset)] $edge_bottom_offset
    $inst setOrient "R90"
    $inst setPlacementStatus "FIRM"

    dict set pad_ring corner_ur [set inst [odb::dbInst_create $block [set corner [get_cell corner ur]] "CORNER_UR"]]
    $inst setOrigin [expr ($chip_width - $edge_right_offset)] [expr ($chip_height - $edge_top_offset)]
    $inst setOrient "R180"
    $inst setPlacementStatus "FIRM"

    dict set pad_ring corner_ul [set inst [odb::dbInst_create $block [set corner [get_cell corner ul]] "CORNER_UL"]]
    $inst setOrigin $edge_left_offset [expr ($chip_height - $edge_top_offset)]
    $inst setOrient "R270"
    $inst setPlacementStatus "FIRM"
  }
  proc get_die_area {} {
    variable footprint
    
    return [dict get $footprint die_area]
  }
  namespace export set_inner_offset set_connect_by_abutment
  namespace export add_cell 
  namespace export set_pad_order set_offsets set_block
  namespace export place_padring init_footprint set_cell_orientation load_footprint

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
