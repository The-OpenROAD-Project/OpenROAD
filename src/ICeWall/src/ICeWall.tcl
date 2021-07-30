sta::define_cmd_args "set_bump_options" {[-pitch pitch] \
                                           [-bump_pin_name pin_name] \
                                           [-spacing_to_edge spacing] \
                                           [-offset {x_offset y_offset}] \
                                           [-array_size {rows columns}] \
                                           [-cell_name bump_cell_table] \
                                           [-num_pads_per_tile value] \
                                           [-rdl_layer name] \
                                           [-rdl_width value] \
                                           [-rdl_spacing value] \
                                           [-rdl_cover_file_name rdl_file_name]}

proc set_bump_options {args} {
  sta::parse_key_args "set_bump_options" args \
    keys {-pitch -bump_pin_name -spacing_to_edge -cell_name -num_pads_per_tile -rdl_layer -rdl_width -rdl_spacing -rdl_cover_file_name -offset -array_size}

  if {[llength $args] > 0} {
    utl::error PAD 218 "Unrecognized arguments ([lindex $args 0]) specified for set_bump_options."
  }
  ICeWall::set_bump_options {*}[array get keys]
}

sta::define_cmd_args "set_bump" {-row row -col col [(-power|-ground|-net) net_name] [-remove]}

proc set_bump {args} {
  if {[ord::get_db_block] == "NULL"} {
    utl::error PAD 231 "Design must be loaded before calling set_bump."
  }

  sta::parse_key_args "set_bump" args \
    keys {-row -col -net -power -ground} \
    flags {-remove}

  if {[llength $args] > 0} {
    utl::error PAD 237 "Unrecognized arguments ([lindex $args 0]) specified for set_bump."
  }

  if {![info exists keys(-row)]} {
    utl::error PAD 233 "Required option -row missing for set_bump."
  }
  if {![info exists keys(-col)]} {
    utl::error PAD 234 "Required option -col missing for set_bump."
  }

  ICeWall::check_rowcol [list row $keys(-row) col $keys(-col)]

  if {([info exists flags(-power)] && [info exists flags(-ground)]) ||
      ([info exists flags(-power)] && [info exists flags(-net)]) ||
      ([info exists flags(-net)] && [info exists flags(-ground)])} {
    utl::error PAD 232 "The -power -ground and -net options are mutualy exclusive for the set_bump command."
  }

  if {[info exists flags(-remove)]} {
    ICeWall::bump_remove $keys(-row) $keys(-col)
  }

  if {[info exists keys(-net)]} {
    ICeWall::bump_set_net $keys(-row) $keys(-col) $keys(-net)
  }
  if {[info exists keys(-power)]} {
    ICeWall::bump_set_power $keys(-row) $keys(-col) $keys(-power)
  }
  if {[info exists keys(-ground)]} {
    ICeWall::bump_set_ground $keys(-row) $keys(-col) $keys(-ground)
  }
}

sta::define_cmd_args "set_padring_options" {[-type (flipchip|wirebond)] \
                                            [-power power_nets] \
                                            [-ground ground_nets] \
                                            [-core_area core_area] \
                                            [-die_area die_area] \
                                            [-offsets offsets] \
                                            [-pad_inst_pattern pad_inst_pattern] \
                                            [-pad_pin_pattern pad_pin_pattern] \
                                            [-pin_layer pin_layer_name] \
                                            [-connect_by_abutment signal_list]}

proc set_padring_options {args} {
  if {[ord::get_db_block] == "NULL"} {
    utl::error PAD 226 "Design must be loaded before calling set_padring_options."
  }

  sta::parse_key_args "set_padring_options" args \
    keys {-type -power -ground -core_area -die_area -offsets -pad_inst_pattern -pad_pin_pattern -pin_layer -connect_by_abutment}

  if {[llength $args] > 0} {
    utl::error PAD 219 "Unrecognized arguments ([lindex $args 0]) specified for set_padring_options."
  }

  if {[info exists keys(-type)]} {
    ICeWall::set_type $keys(-type)
  }

  if {[info exists keys(-power)]} {
    ICeWall::add_power_nets {*}$keys(-power)
  }

  if {[info exists keys(-ground)]} {
    ICeWall::add_ground_nets {*}$keys(-ground)
  }

  if {[info exists keys(-core_area)]} {
    ICeWall::set_core_area {*}$keys(-core_area)
  }

  if {[info exists keys(-die_area)]} {
    ICeWall::set_die_area {*}$keys(-die_area)
  }

  if {[info exists keys(-offsets)]} {
    ICeWall::set_offsets $keys(-offsets)
  }

  if {[info exists keys(-pad_inst_pattern)]} {
    ICeWall::set_pad_inst_name $keys(-pad_inst_pattern)
  }

  if {[info exists keys(-pad_pin_pattern)]} {
    ICeWall::set_pad_pin_name $keys(-pad_pin_pattern)
  }

  if {[info exists keys(-pin_layer)]} {
    ICeWall::set_pin_layer $keys(-pin_layer)
  }

  if {[info exists keys(-connect_by_abutment)]} {
    ICeWall::set_library_connect_by_abutment {*}$keys(-connect_by_abutment)
  }
}

sta::define_cmd_args "define_pad_cell" {[-name name] \
                                      [-type cell_type|-fill|-corner|-bondpad|-bump] \
                                      [-cell_name cell_names_per_side] \
                                      [-orient orientation_per_side] \
                                      [-pad_pin_name pad_pin_name] \
                                      [-break_signals signal_list] \
                                      [-physical_only]}

proc define_pad_cell {args} {
  sta::parse_key_args "define_pad_cell" args \
    keys {-name -type -cell_name -orient -pad_pin_name -break_signals} \
    flags {-fill -corner -bondpad -bump -physical_only}

  if {![ord::db_has_tech]} {
    utl::error PAD 225 "Library must be loaded before calling define_pad_cell."
  }

  if {[llength $args] > 0} {
    utl::error PAD 220 "Unrecognized arguments ([lindex $args 0]) specified for define_pad_cell."
  }
  set args [array get keys]
  if {[info exists flags(-physical_only)]} {
    dict set args -physical_only 1
  }

  foreach flag {-fill -corner -bondpad -bump} {
    if {[info exists flags($flag)]} {
      if {[dict exists $args "-type"]} {
        utl::error PAD 208 "Type option already set to [dict get $args -type], option $flag cannot be used to reset the type."
      }
      dict set args -type [regsub -- {\-} $flag {}]
    }
  }

  ICeWall::add_libcell {*}$args
}

sta::define_cmd_args "add_pad" {[-name name] \
                                  [-type type] \
                                  [-cell library_cell] \
                                  [-signal signal_name] \
                                  [-edge edge] \
                                  [-location location] \
                                  [-bump rowcol] \
                                  [-bondpad bondpad] \
                                  [-inst_name inst_name]}

proc add_pad {args} {
  sta::parse_key_args "add_pad" args \
    keys {-name -type -cell -signal -edge -location -bump -bondpad -inst_name}

  if {[ord::get_db_block] == "NULL"} {
    utl::error PAD 224 "Design must be loaded before calling add_pad."
  }

  if {[llength $args] > 0} {
    utl::error PAD 221 "Unrecognized arguments ([lindex $args 0]) specified for add_pad."
  }
  ICeWall::add_pad {*}[array get keys]
}

sta::define_cmd_args "initialize_padring" {[-signal_assignment_file signal_assigment_file]}
proc initialize_padring {args} {
  if {[ord::get_db_block] == "NULL"} {
    utl::error PAD 227 "Design must be loaded before calling initialize_padring."
  }

  sta::parse_key_args "initialize_padring" args \
    keys {-signal_assignment_file}

  if {[llength $args] > 0} {
    utl::error PAD 222 "Unrecognized arguments ([lindex $args 0]) specified for initialize_padring."
  }
  ICeWall::init_footprint {*}[array get keys]
}

namespace eval ICeWall {
  variable footprint {}
  variable library {}
  variable cells {}
  variable block {}
  variable db {}
  variable default_orientation {bottom R0 right R90 top R180 left R270 ll R0 lr MY ur R180 ul MX}
  variable connect_pins_by_abutment
  variable idx {fill 0}
  variable unassigned_idx 0

  proc initialize {} {
    variable db
    variable tech
    variable block

    set db [::ord::get_db]
    set tech [$db getTech]
    set block [[$db getChip] getBlock]
    init_process_footprint
  }

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

  proc set_library_connect_by_abutment {args} {
    variable library

    dict set library connect_by_abutment $args

    if {[dict exists $library breakers]} {
      foreach breaker_cell_type [dict get $library breakers] {
        if {![dict exists $library types $breaker_cell_type]} {
          utl::error PAD 203 "No cell type $breaker_cell_type defined."
        }
        if {![dict exists $library cells [dict get $library types $breaker_cell_type]]} {
          utl::error PAD 204 "No cell [dict get $library types $breaker_cell_type] defined."
        }
        foreach break_signal [dict keys [dict get $library cells [dict get $library types $breaker_cell_type] breaks]] {
          if {[lsearch [dict get $library connect_by_abutment] $break_signal] == -1} {
            utl::error PAD 187 "Signal $break_signal not defined in the list of signals to connect by abutment."
          }
        }
      }
    }
  }

  proc get_origin {center width height orient} {
      if {![dict exists $center x]} {
        utl::error PAD 54 "Parameter center \"$center\" missing a value for x."
      }
      if {![dict exists $center y]} {
        utl::error PAD 55 "Parameter center \"$center\" missing a value for y."
      }
      switch -exact $orient {
        R0    {
          set x [expr [dict get $center x] - ($width / 2)]
          set y [expr [dict get $center y] - ($height / 2)]
        }
        R180  {
          set x [expr [dict get $center x] + ($width / 2)]
          set y [expr [dict get $center y] + ($height / 2)]
        }
        MX    {
          set x [expr [dict get $center x] - ($width / 2)]
          set y [expr [dict get $center y] + ($height / 2)]
        }
        MY    {
          set x [expr [dict get $center x] + ($width / 2)]
          set y [expr [dict get $center y] - ($height / 2)]
        }
        R90   {
          set x [expr [dict get $center x] + ($height / 2)]
          set y [expr [dict get $center y] - ($width / 2)]
        }
        R270  {
          set x [expr [dict get $center x] - ($height / 2)]
          set y [expr [dict get $center y] + ($width / 2)]
        }
        MXR90 {
          set x [expr [dict get $center x] + ($height / 2)]
          set y [expr [dict get $center y] + ($width / 2)]
        }
        MYR90 {
          set x [expr [dict get $center x] - ($height / 2)]
          set y [expr [dict get $center y] - ($width / 2)]
        }
        default {utl::error "PAD" 5 "Illegal orientation \"$orient\" specified."}
      }

      return [list x $x y $y]
  }

  proc get_center {center width height orient} {
      if {![dict exists $center x]} {
        utl::error PAD 56 "Parameter center \"$center\" missing a value for x."
      }
      if {![dict exists $center y]} {
        utl::error PAD 57 "Parameter center \"$center\" missing a value for y."
      }
      switch -exact $orient {
        R0    {
          set x [expr [dict get $center x] + ($width / 2)]
          set y [expr [dict get $center y] + ($height / 2)]
        }
        R180  {
          set x [expr [dict get $center x] - ($width / 2)]
          set y [expr [dict get $center y] - ($height / 2)]
        }
        MX    {
          set x [expr [dict get $center x] + ($width / 2)]
          set y [expr [dict get $center y] - ($height / 2)]
        }
        MY    {
          set x [expr [dict get $center x] - ($width / 2)]
          set y [expr [dict get $center y] + ($height / 2)]
        }
        R90   {
          set x [expr [dict get $center x] - ($height / 2)]
          set y [expr [dict get $center y] + ($width / 2)]
        }
        R270  {
          set x [expr [dict get $center x] + ($height / 2)]
          set y [expr [dict get $center y] - ($width / 2)]
        }
        MXR90 {
          set x [expr [dict get $center x] - ($height / 2)]
          set y [expr [dict get $center y] - ($width / 2)]
        }
        MYR90 {
          set x [expr [dict get $center x] + ($height / 2)]
          set y [expr [dict get $center y] + ($width / 2)]
        }
        default {utl::error "PAD" 6 "Illegal orientation \"$orient\" specified."}
      }

      return [list x $x y $y]
  }

  proc get_footprint_padcells_by_side {side_name} {
    variable footprint
    if {![dict exists $footprint order $side_name]} {
      set padcells {}
      if {![dict exists $footprint padcell]} {
        utl::error PAD 58 "Footprint has no padcell attribute."
      }
      dict for {padcell data} [dict get $footprint padcell] {
        if {![dict exists $data side]} {
          utl::error PAD 59 "No side attribute specified for padcell $padcell."
        }
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

    utl::error "PAD" 24 "Cannot find bondpad type in library."
  }

  proc get_library_bondpad_height {} {
    variable library

    if {[dict exists $library types bondpad]} {
      set bondpad_cell [get_cell "bondpad" "top"]
      return [$bondpad_cell getHeight]
    }

    utl::error "PAD" 26 "Cannot find bondpad type in library."
  }

  proc get_footprint_padcell_names {} {
    variable footprint
    if {![dict exists $footprint padcell]} {
      dict set footprint padcell {}
    }
    return [dict keys [dict get $footprint padcell]]
  }

  proc get_footprint_padcell_order {} {
    variable footprint
    if {![dict exists $footprint full_order]} {
      dict set footprint full_order {}
    }
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

    if {[dict exists $footprint create_padcells]} {
      if {[dict get $footprint create_padcells]} {
        return 1
      }
    }
    return 0
  }

  proc is_footprint_defined {parameter} {
    variable footprint

    return [dict exists $footprint $parameter]
  }

  proc get_padcell_inst_info {padcell} {
    variable footprint

    if {[llength $padcell] > 1} {
      set inst $padcell
    } elseif {[dict exists $footprint padcell $padcell]} {
      set inst [dict get $footprint padcell $padcell]
    } else {
      # debug $padcell
      utl::error "PAD" 25 "No instance found for $padcell."
    }

    return  $inst
  }

  proc get_scaled_location {padcell} {
    variable footprint

    if {[dict exists $footprint padcell $padcell scaled_center]} {
      return [dict get $footprint padcell $padcell scaled_center]
    } elseif {[dict exists $footprint padcell $padcell scaled_origin]} {
      return [dict get $footprint padcell $padcell scaled_origin]
    } elseif {[dict exists $footprint padcell $padcell cell center]} {
      return [get_scaled_center $padcell cell]
    } elseif {[dict exists $footprint padcell $padcell cell origin]} {
      return [get_scaled_origin $padcell cell]
    }

    utl::error PAD 60 "Cannot determine location of padcell $padcell."
  }

  proc get_scaled_origin {padcell {type "cell"}} {
    variable library
    # debug "padcell: $padcell"

    set inst [get_padcell_inst_info $padcell]
    # debug $inst

    if {[dict exists $inst $type scaled_origin]} {
      set scaled_origin [dict get $inst $type scaled_origin];
    } elseif {[dict exists $inst $type origin]} {
      set scaled_origin [list \
        x [ord::microns_to_dbu [dict get $inst $type origin x]] \
        y [ord::microns_to_dbu [dict get $inst $type origin y]] \
      ]
    } else {
      # debug "$padcell $type $inst"
      utl::error PAD 114 "No origin information specified for padcell $padcell $type $inst."
    }

    # debug "end: $scaled_origin"
    return $scaled_origin
  }

  proc get_scaled_center {padcell {type cell}} {
    variable library

    set inst [get_padcell_inst_info $padcell]

    if {[dict exists $inst $type scaled_center]} {
      set scaled_center [dict get $inst $type scaled_center];
    } elseif {[dict exists $inst $type center]} {
      set scaled_center [list \
        x [ord::microns_to_dbu [dict get $inst $type center x]] \
        y [ord::microns_to_dbu [dict get $inst $type center y]] \
      ]
    } else {
      utl::error PAD 115 "No origin information specified for padcell $padcell."
    }

    return $scaled_center
  }

  proc get_padcell_scaled_origin {padcell {type cell}} {
    variable footprint

    if {![dict exists $footprint padcell $padcell $type scaled_origin]} {
      dict set footprint padcell $padcell $type scaled_origin [get_scaled_origin $padcell $type]
    }

    return [dict get $footprint padcell $padcell $type scaled_origin]
  }

  proc get_padcell_origin {padcell {type cell}} {
    set scaled_origin [get_padcell_scaled_origin $padcell]

    return [list x [ord::dbu_to_microns [dict get $scaled_origin x]] y [ord::dbu_to_microns [dict get $scaled_origin y]]]
  }

  proc get_padcell_scaled_center {padcell {type cell}} {
    variable footprint

    if {![dict exists $footprint padcell $padcell $type scaled_center]} {
      dict set footprint padcell $padcell $type scaled_center [get_scaled_center $padcell $type]
    }

    return [dict get $footprint padcell $padcell $type scaled_center]
  }

  proc get_die_area {} {
    variable footprint

    if {![dict exists $footprint die_area]} {
      if {[dict exists $footprint scaled_die_area]} {
        set die_area {}
        foreach value [dict get $footprint scaled_die_area] {
          lappend die_area [ord::dbu_to_microns $value]
        }
        dict set footprint die_area $die_area
      } else {
        if {[set block [ord::get_db_block]] != "NULL"} {
          dict set footprint die_area [ord::get_die_area]
          # debug [dict get $footprint die_area]
        } else {
          utl::error "PAD" 31 "No die_area specified in the footprint specification."
        }
      }
    }
    return [dict get $footprint die_area]
  }

  proc get_core_area {} {
    variable footprint

    if {![dict exists $footprint core_area]} {
      if {[array names ::env CORE_AREA] != ""} {
        dict set footprint core_area $::env(CORE_AREA)
      } else {
        utl::error "PAD" 41 "A value for core_area must specified in the footprint specification, or in the environment variable CORE_AREA."
      }
    }
    return [dict get $footprint core_area]
  }

  proc set_scaled_die_area {xMin yMin xMax yMax} {
    variable footprint

    dict set footprint scaled_die_area [list $xMin $yMin $xMax $yMax]
  }

  proc get_scaled_die_area {} {
    variable footprint

    if {![dict exists $footprint scaled_die_area]} {
      set area {}
      foreach value [get_die_area] {
        lappend area [ord::microns_to_dbu $value]
      }
      dict set footprint scaled_die_area $area
    }
    return [dict get $footprint scaled_die_area]
  }

  proc get_footprint_die_size_x {} {
    variable footprint

    if {![dict exists $footprint die_area]} {
      get_die_area
      if {![dict exists $footprint die_area]} {
        utl::error PAD 61 "Footprint attribute die_area has not been defined."
      }
    }

    return [ord::microns_to_dbu [expr [lindex [dict get $footprint die_area] 2] - [lindex [dict get $footprint die_area] 0]]]
  }

  proc get_footprint_die_size_y {} {
    variable footprint

    if {![dict exists $footprint die_area]} {
      utl::error PAD 62 "Footprint attribute die_area has not been defined."
    }
    return [ord::microns_to_dbu [expr [lindex [dict get $footprint die_area] 3] - [lindex [dict get $footprint die_area] 1]]]
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

    if {![dict exists $footprint core_area]} {
      dict set footprint core_area [list \
        [expr $edge_left_offset + $corner_width + $inner_left_offset] \
        [expr $edge_bottom_offset + $corner_width + $inner_bottom_offset] \
        [expr $chip_width - $edge_right_offset - $corner_width - $inner_right_offset] \
        [expr $chip_height - $edge_top_offset - $corner_width - $inner_top_offset] \
      ]
    }
    return [dict get $footprint core_area]
  }

  proc set_scaled_core_area {args} {
    variable footprint

    dict set footprint scaled_core_area {*}$args
  }

  proc get_scaled_core_area {} {
    variable footprint

    if {![dict exists $footprint scaled_core_area]} {
      utl::error "PAD" 16 "Scaled core area not defined."
    }

    return [dict get $footprint scaled_core_area]
  }

  proc get_side_name {x y} {
    switch [pdngen::get_quadrant [get_scaled_die_area] $x $y] {
      "b" {return bottom}
      "r" {return right}
      "t" {return top}
      "l" {return left}
    }
  }

  proc get_padcell_side_name {padcell} {
    variable footprint

    if {![dict exists $footprint "padcell" $padcell side]} {
      utl::error PAD 116 "Side for padcell $padcell cannot be determined."
    }

    return [dict get $footprint padcell $padcell side]
  }

  proc get_side_from_orient {cell_ref orient} {
    variable library

    if {[dict exists $library cells $cell_ref]} {
      if {[dict exists $library cells $cell_ref orient]} {
        foreach side [dict keys [dict get $library cells $cell_ref orient]] {
          if {[dict get $library cells $cell_ref orient $side] == $orient} {
            return $orient
          }
        }
        utl::error PAD 117 "No orient entry for cell reference $cell_ref matching orientation $orient."
      } else {
        return $orient
      }
    } else {
      utl::error PAD 119 "No cell reference $cell_ref found in library data."
    }
  }

  proc set_padcell_type {padcell type} {
    variable footprint

    dict set footprint padcell $padcell type $type
  }

  proc set_padcell_cell_name {padcell cell_name} {
    variable footprint

    dict set footprint padcell $padcell cell_name $cell_name
  }

  proc set_padcell_side_name {padcell side_name} {
    variable footprint

    dict set footprint padcell $padcell side $side_name
  }
  
  proc set_padcell_center {padcell x y } {
    variable footprint

    dict set footprint padcell $padcell cell center x $x
    dict set footprint padcell $padcell cell center y $y
  }


  proc set_padcell_scaled_center {padcell x y } {
    variable footprint

    dict set footprint padcell $padcell cell scaled_center x $x
    dict set footprint padcell $padcell cell scaled_center y $y
  } 
   
  proc set_padcell_location {padcell location} {
    variable footprint

    dict set footprint padcell $padcell cell $location
  }

  proc set_padcell_bondpad {padcell bondpad} {
    variable footprint

    dict set footprint padcell $padcell bondpad $bondpad
  }

  proc get_padcell_type {padcell} {
    variable footprint
    # debug "start"

    if {[llength $padcell] == 1} {
      set padcell_name $padcell
      if {![dict exists $footprint padcell $padcell_name]} {
        utl::error PAD 63 "Padcell $padcell_name not specified."
      }
      set padcell [dict get $footprint padcell $padcell]
    }

    if {![dict exists $padcell type]} {
      utl::error PAD 64 "No type attribute specified for padcell $padcell_name."
    }
    # debug "end"
    return [dict get $padcell type]
  }

  proc get_padcell_inst_name {padcell} {
    variable footprint

    if {[dict exists $footprint padcell $padcell inst] && [dict get $footprint padcell $padcell inst] != "NULL"} {
      set inst_name [[dict get $footprint padcell $padcell inst] getName]
    } elseif {[dict exists $footprint padcell $padcell pad_inst_name]} {
      set inst_name [format [dict get $footprint padcell $padcell pad_inst_name] [get_padcell_assigned_name $padcell]]
    } else {
      if {[is_padcell_power $padcell] || [is_padcell_ground $padcell]} {
        set padcell_assigned_name $padcell
      } else {
        set padcell_assigned_name [get_padcell_assigned_name $padcell]
      }
      if {[dict exists $footprint pad_inst_name]} {
        set inst_name [format [dict get $footprint pad_inst_name] $padcell_assigned_name]
      } else {
        set inst_name "u_$padcell_assigned_name"
      }
    }

    # debug "inst_name $inst_name"
    return $inst_name
  }

  proc set_padcell_inst {padcell inst} {
    variable footprint

    dict set footprint padcell $padcell inst $inst
  }

  proc is_padcell_signal_type {padcell} {
    variable footprint

    # debug "start: $padcell"
    if {![dict exists $footprint padcell $padcell signal_type]} {
      if {![dict exists $footprint padcell $padcell type]} {
        utl::error PAD 65 "No type attribute specified for padcell $padcell."
      }
      set type [dict get $footprint padcell $padcell type]
      if {$type == "sig"} {
        dict set footprint padcell $padcell signal_type 1
      } else {
        # debug "check library type $type for padcell $padcell"
        dict set footprint padcell $padcell signal_type [is_library_cell_signal_type $type]
      }
    }

    # debug "Return value: [dict get $footprint padcell $padcell signal_type]"
    return [dict get $footprint padcell $padcell signal_type]
  }

  proc is_library_cell_signal_type {type} {
    variable library

    # debug "start: $type"
    if {[dict exists $library types $type]} {
      set cell_ref [dict get $library types $type]

      if {![dict exists $library cells $cell_ref signal_type]} {
        dict set library cells $cell_ref signal_type 0
      }
      # debug "Return attribute: [dict get $library cells $cell_ref signal_type]"
      return [dict get $library cells $cell_ref signal_type]
    }

    # debug "Type $type not found"
    return 0
  }

  proc get_padcell_inst {padcell} {
    variable footprint
    variable block
    # debug start

    if {![dict exists $footprint padcell $padcell inst]} {
      set padcell_inst_name [get_padcell_inst_name $padcell]
      # debug "Looking for padcell with inst name $padcell_inst_name"
      if {[set inst [$block findInst $padcell_inst_name]] != "NULL"} {
        set signal_name [get_padcell_signal_name $padcell]
        # debug "Pad match by name for $padcell ($signal_name)"
        set_padcell_inst $padcell [$block findInst [get_padcell_inst_name $padcell]]
      } elseif {[is_padcell_signal_type $padcell]} {
        set signal_name [get_padcell_signal_name $padcell]
        # debug "Try signal matching - signal: $signal_name"
        if {[is_padcell_unassigned $padcell]} {
          # debug "Pad unassigned for $padcell"
          set_padcell_inst $padcell "NULL"
        } elseif {[set net [$block findNet $signal_name]] != "NULL"} {
          # debug "Pad match by net for $padcell ($signal_name)"
          set net [$block findNet $signal_name]
          if {$net == "NULL"} {
            utl::error "PAD" 32 "Cannot find net $signal_name for $padcell in the design."
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
    if {![dict exists $library types $type]} {
      utl::error PAD 66 "Library data has no type entry $type."
    }
    return [dict get $library types $type]
  }

  proc get_library_cell_offset {cell_name} {
    variable library

    if {![dict exists $library cells $cell_name scaled_offset]} {
      if {[dict exists $library cells $cell_name offset]} {
        set offset [dict get $library cells $cell_name offset]
        dict set library cells $cell_name scaled_offset [list [ord::microns_to_dbu [lindex $offset 0]] [ord::microns_to_dbu [lindex $offset 1]]]
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

    set cell_ref [get_library_cell_by_type $type]
    if {[dict exists $library cells $cell_ref]} {
      if {[dict exists $library cells $cell_ref cell_name]} {
        if {[llength [dict get $library cells $cell_ref cell_name]] > 1} {
          # debug [dict get $library cells $cell_ref cell_name]
          if {![dict exists $library cells $cell_ref cell_name $position]} {
            utl::error PAD 161 "Position $position not defined for $cell_ref, expecting one of [join [dict keys [dict get $library cells $cell_ref cell_name]] {, }]."
          }
          set cell_name [dict get $library cells $cell_ref cell_name $position]
        } else {
          set cell_name [dict get $library cells $cell_ref cell_name]
        }
      } else {
        set cell_name $cell_ref
      }
    } else {
      set cell_name $cell_ref
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

    if {[llength $padcell] == 1} {
      if {[dict exists $footprint padcell $padcell]} {
        set cell_type [get_padcell_type $padcell]
        set side [get_padcell_side_name $padcell]
      }
    } else {
      set cell_type [get_padcell_type $padcell]
      if {[dict exists $padcell side]} {
        set side [dict get $padcell side]
      } else {
        set side "none"
      }
    }

    return [get_library_cell_name $cell_type $side]
  }

  proc get_padcell_assigned_name {padcell} {
    variable footprint

    if {[dict exists $footprint padcell $padcell use_signal_name]} {
      return [dict get $footprint padcell $padcell use_signal_name]
    }
    return "$padcell"
  }

  proc get_cell_master {name} {
    variable db

    if {[set cell [$db findMaster $name]] != "NULL"} {return $cell}

    utl::error "PAD" 8 "Cannot find cell $name in the database."
  }

  proc get_library_cell_orientation {cell_type position} {
    variable library

    # debug "cell_type $cell_type position $position"
    if {![dict exists $library cells $cell_type orient $position]} {
      if {![dict exists $library cells]} {
        utl::error "PAD" 49 "No cells defined in the library description."
      } elseif {![dict exists $library cells $cell_type]} {
        utl::error "PAD" 96 "No cell $cell_type defined in library ([dict keys [dict get $library cells]])."
      } else {
        utl::error "PAD" 97 "No entry found in library definition for cell $cell_type on $position side."
      }
    }

    set orient [dict get $library cells $cell_type orient $position]
  }

  proc get_library_cell_parameter_default {cell_name parameter_name} {
    variable library

    if {![dict exists $library cells $cell_name parameter_defaults $parameter_name]} {
      dict set library cells $cell_name parameter_defaults $parameter_name ""
    }
    return [dict get $library cells $cell_name parameter_defaults $parameter_name]
  }

  proc get_padcell_orient {padcell {element "cell"}} {
    variable footprint

    if {![dict exists $footprint padcell $padcell $element]} {
      utl::error PAD 120 "Padcell $padcell does not have any location information to derive orientation."
    } else {
      if {![dict exists $footprint padcell $padcell $element orient]} {
        utl::error PAD 121 "Padcell $padcell does not define orientation for $element."
      } else {
        return [dict get $footprint padcell $padcell $element orient]
      }
    }

    return [dict get $padcell $element orient]
  }

  proc get_cell {type {position "none"}} {
    # debug "cell_name [get_library_cell_name $type $position]"
    return [get_cell_master [get_library_cell_name $type $position]]
  }

  proc get_cells {type side} {
    variable library

    set cell_list {}
    if {![dict exists $library types $type]} {
      utl::error PAD 70 "Library does not have type $type specified."
    }
    foreach cell_ref [dict get $library types $type] {
      if {[dict exists $library cells $cell_ref cell_name]} {
        if {[dict exists $library cells $cell_ref cell_name $side]} {
          set cell_name [dict get $library cells $cell_ref cell_name $side]
        } else {
          set cell_name $cell_ref
        }
      } else {
        set cell_name $cell_ref
      }
      set master [get_cell_master $cell_name]
      if {$master == "NULL"} {
        utl::error PAD 71 "No cell $cell_name found."
      }
      dict set cell_list $cell_ref master $master
    }
    return $cell_list
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
      utl::error "PAD" 33 "No value defined for pad_pin_name in the library or cell data for $type."
    }
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

  proc new_padcell {padcell} {
    variable footprint

    dict set footprint padcell $padcell {}
  }

  proc set_padcell_signal_name {padcell signal_name} {
    variable footprint
    variable block

    # debug "start: padcell $padcell, signal_name: $signal_name"
    if {[dict exists $footprint padcell $padcell]} {
      dict set footprint padcell $padcell signal_name $signal_name
      dict set footprint padcell $padcell [check_signal_name [dict get $footprint padcell $padcell]]
      return [get_padcell_signal_name $padcell]
    } else {
      return ""
    }
  }

  proc get_padcell_signal_name {padcell} {
    variable footprint

    if {![dict exists $footprint padcell $padcell]} {
      # debug "padcell: $padcell cells: [dict keys [dict get $footprint padcell]]"
      utl::error "PAD" 22 "Cannot find padcell $padcell."
    }
    # debug [dict get $footprint padcell $padcell]
    if {![dict exists $footprint padcell $padcell use_signal_name]} {
      # debug [dict get $footprint padcell $padcell]
      utl::error "PAD" 23 "Signal name for padcell $padcell has not been set."
    }

    return [dict get $footprint padcell $padcell use_signal_name]
  }

  proc get_library_min_bump_spacing_to_die_edge {} {
    variable library

    if {[dict exists $library bump spacing_to_edge]} {
      return [ord::microns_to_dbu [expr [dict get $library bump spacing_to_edge]]]
    }

    utl::error "PAD" 21 "Value of bump spacing_to_edge not specified."
  }

  proc get_footprint_min_bump_spacing_to_die_edge {} {
    variable footprint

    if {![dict exists $footprint scaled bump_spacing_to_edge]} {
      if {[dict exists $footprint bump spacing_to_edge]} {
        dict set footprint scaled bump_spacing_to_edge [ord::microns_to_dbu [dict get $footprint bump spacing_to_edge]]
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
      dict set footprint padcell $padcell rdl_trace ""
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

    if {![dict exists $footprint padcell $padcell bump]} {
      utl::error PAD 72 "No bump attribute for padcell $padcell."
    }
    if {![dict exists $footprint padcell $padcell bump name]} {
      if {![dict exists $footprint padcell $padcell bump row]} {
        utl::error PAD 73 "No row attribute specified for bump associated with padcell $padcell."
      }
      if {![dict exists $footprint padcell $padcell bump col]} {
        utl::error PAD 74 "No col attribute specified for bump associated with padcell $padcell."
      }
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

    if {![dict exists $footprint bump]} {
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

    if {![dict exists $footprint padcell $padcell bump]} {
      utl::error PAD 75 "No bump attribute for padcell $padcell."
    }
    if {![dict exists $footprint padcell $padcell bump row]} {
      utl::error PAD 76 "No row attribute specified for bump associated with padcell $padcell."
    }
    if {![dict exists $footprint padcell $padcell bump col]} {
      utl::error PAD 77 "No col attribute specified for bump associated with padcell $padcell."
    }
    set row [dict get $footprint padcell $padcell bump row]
    set col [dict get $footprint padcell $padcell bump col]

    return [get_bump_origin $row $col]
  }

  proc get_bump_center {row col} {
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

    set center [get_bump_center $row $col]
    set bump_width [get_library_bump_width]
    # debug "bump_width : [expr $bump_width / 2000.0]"

    return [list \
      x [expr [dict get $center x] - $bump_width / 2] \
      y [expr [dict get $center y] - $bump_width / 2] \
    ]
  }

  proc get_bump_signal_name {row col} {
    variable footprint

    set padcell [get_padcell_at_row_col $row $col]

    if {$padcell != ""} {
      return [get_padcell_signal_name $padcell]
    }

    if {[set name [bump_get_net $row $col]] != ""} {
      return [bump_get_net $row $col]
    }

    set rdl_routing_layer [get_footprint_rdl_layer_name]
    if {[pdngen::get_dir $rdl_routing_layer] == "hor"} {
      if {$row % 2 == 0} {
        return "VDD"
      }
    } else {
      if {$col % 2 == 0} {
        return "VDD"
      }
    }

    return "VSS"
  }

  proc has_padcell_bondpad {padcell} {
    variable footprint

    return [dict exists $footprint padcell $padcell bondpad]
  }

  proc get_padcell_bondpad_scaled_origin {padcell} {
    return [get_padcell_scaled_origin $padcell bondpad]
  }
  proc get_padcell_bondpad_origin {padcell} {
    return [get_padcell_origin $padcell bondpad]
  }

  proc get_padcell_bondpad_center {padcell} {
    return [get_padcell_scaled_center $padcell bondpad]
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
      return [expr {[dict get $footprint Type] == "Wirebond" || [dict get $footprint Type] == "wirebond"}]
    }

    if {[dict exists $footprint type]} {
      return [expr {[dict get $footprint type] == "Wirebond" || [dict get $footprint type] == "wirebond"}]
    }

    return 0
  }

  proc is_footprint_flipchip {} {
    variable footprint

    if {[dict exists $footprint Type]} {
      return [expr {[dict get $footprint Type] == "Flipchip" || [dict get $footprint Type] == "flipchip"}]
    }

    if {[dict exists $footprint type]} {
      return [expr {[dict get $footprint type] == "Flipchip" || [dict get $footprint type] == "flipchip"}]
    }

    return 0
  }

  proc init_library_bondpad {} {
    variable library
    variable bondpad_width
    variable bondpad_height

    # debug "[dict get $library types]"
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
          set origin [get_padcell_scaled_origin $padcell]

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
          utl::warn "PAD" 11 "Expected instance $name for padcell, $padcell not found."
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
      utl::error "PAD" 2 "Not enough bumps: available [expr 2 * ($num_signals_top_bottom + $num_signals_left_right)], required $required."
    }
    # debug "available [expr 2 * ($num_signals_top_bottom + $num_signals_left_right)], required $required"

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

  proc read_signal_assignments {signal_assignment_file} {
    if {![file exists $signal_assignment_file]} {
      utl::error "PAD" 7 "File $signal_assignment_file not found."
    }
    set errors {}
    set ch [open $signal_assignment_file]
    while {![eof $ch]} {
      set line [gets $ch]
      set line [regsub {\#.} $line {}]
      if {[llength $line] == 0} {continue}
      # debug "$line"
      set line [regsub -all {\s+} $line " "]
      set line [regsub -all {\s+$} $line ""]

      set pad_name [lindex [split $line] 0]
      set signal_name [lindex [split $line] 1]
      # debug "padcell: $pad_name, signal_name: $signal_name"
      if {[set_padcell_signal_name $pad_name $signal_name] == ""} {
        lappend errors "Pad id $pad_name not found in footprint"
      } else {
        # debug "properties: [lrange $line 2 end]"
        dict for {key value} [lrange $line 2 end] {
          set_padcell_property $pad_name $key $value
        }
      }
    }

    if {[llength $errors] > 0} {
      set str "\n"
      foreach msg $errors {
         set str "$str\n  $msg"
      }
      utl::error "PAD" 1 "$str\nIncorrect signal assignments ([llength $errors]) found."
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
      if {[llength $cell_name] > 1} {
        if {[dict exists $library cells $cell_name cell_name]} {
          dict for {key actual_cell_name} [dict get $library cells $cell_name cell_name] {
            lappend cells $actual_cell_name
          }
        } else {
          lappend cells $cell_name
        }
      } else {
        lappend cells $cell_name
      }
    }
    return $cells
  }

  proc get_library_cells_in_design {} {
    variable block

    set library_cells [get_library_cells]
    set existing_io_components {}

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
    variable block

    foreach io_signal [get_design_io] {

    }
  }

  proc load_library_file {library_file} {
    # debug "starti: $library_file"
    source $library_file
    # debug "end"
  }

  proc inst_compare_x {inst1 inst2} {
    return [expr [lindex [$inst1 getOrigin] 0] - [lindex [$inst2 getOrigin] 0]]
  }

  proc inst_compare_y {inst1 inst2} {
    return [expr [lindex [$inst1 getOrigin] 1] - [lindex [$inst2 getOrigin] 1]]
  }

  proc set_footprint_padcell_inst {padcell_name inst} {
    variable footprint

    set_padcell_inst $padcell_name $inst
    dict set footprint padcell $padcell_name cell scaled_origin x [lindex [$inst getOrigin] 0]
    dict set footprint padcell $padcell_name cell scaled_origin y [lindex [$inst getOrigin] 1]
  }

  proc get_padcell_design_signal_name {padcell} {
    set inst [get_padcell_inst $padcell]

    if {$inst == "NULL"} {
      utl::error "PAD" 141 "Signal name for padcell $padcell has not been set."
    }
    if {[is_padcell_signal_type $padcell]} {
      # debug "Get iterms for $padcell"
      set pin_name [get_padcell_pad_pin_name $padcell]
      # debug "pin_name: $pin_name"
      set iterm [$inst findITerm $pin_name]
      # debug "iterm: $iterm"
      if {$iterm != "NULL"} {
        set net [$iterm getNet]
        if {[llength [set pad_connections [$net getBTerms]]] == 1} {
          set_padcell_signal_name $padcell [$pad_connections getName]
        } else {
          utl::error "PAD" 17 "Found [llength $pad_connections] top level connections to $pin_name of padcell i$padcell (inst:[$inst getName]), expecting only 1."
        }
      }
    } else {
       # For non-signal based pads, use the instance name for assignment
      set_padcell_signal_name $padcell [$inst getName]
    }
  }

  proc extract_footprint {} {
    variable db
    variable tech
    variable block

    # debug "start"
    set db [::ord::get_db]
    set tech [$db getTech]
    set block [ord::get_db_block]

    # debug "cells"
    set cells [get_library_cells_in_design]

    set die_area [$block getBBox]
    set core_area [ord::get_db_core]
    # debug "die_area: $die_area"

    set_footprint_offsets 0

    set_scaled_die_area [$die_area xMin] [$die_area yMin] [$die_area xMax] [$die_area yMax]
    if {[llength [$block getRows]] > 0} {
      set_scaled_core_area [pdngen::get_core_area]
    } else {
    }
    # debug "scaled_die_area [$die_area xMin] [$die_area yMin] [$die_area xMax] [$die_area yMax]"

    set count 0
    # debug "Design insts [llength [$block getInsts]]"
    foreach inst [$block getInsts] {
      set type [get_library_inst_type $inst]
      if {$type == "none"} {continue}
      if {$type == "fill" || $type == "corner"} {continue}
      set inst_bbox [$inst getBBox]
      set inst_center [list [expr ([$inst_bbox xMax] + [$inst_bbox xMin]/2)] [expr ([$inst_bbox yMax] + [$inst_bbox yMin]) / 2]]
      set side_name [get_side_name {*}$inst_center]
      lappend pads($side_name) $inst
      incr count
    }
    # debug "Instances: $count"

    foreach type [get_library_types] {
      set index($type) 0
    }

    foreach side {bottom right top left} {
      set error_found 0
      if {[array names pads $side] == ""} {
        set error_found 1
        utl::warn "PAD" 42 "Cannot find any pads on $side side."
      }
      if {$error_found == 1} {
        utl::error "PAD" 43 "Pads must be defined on all sides of the die for successful extraction."
      }
    }

    set xMin [$die_area xMax]
    set yMin [$die_area yMax]
    set xMax [$die_area xMin]
    set yMax [$die_area yMin]
    # set offset
    foreach pad $pads(bottom) {
      set bbox [$pad getBBox]
      if {[set yMinBox [$bbox yMin]] < $yMin} {
        set yMin $yMinBox
      }
    }
    foreach pad $pads(right) {
      set bbox [$pad getBBox]
      if {[set xMaxBox [$bbox xMax]] > $xMax} {
        set xMax $xMaxBox
      }
    }
    foreach pad $pads(top) {
      set bbox [$pad getBBox]
      if {[set yMaxBox [$bbox yMax]] > $yMax} {
        set yMax $yMaxBox
      }
    }
    foreach pad $pads(left) {
      set bbox [$pad getBBox]
      if {[set xMinBox [$bbox xMin]] < $xMin} {
        set xMin $xMinBox
      }
    }

    # debug "# pads: [llength $pads(bottom)] [llength $pads(right)] [llength $pads(top)] [llength $pads(left)]"
    # debug "die_area: [$die_area xMin] [$die_area yMin] [$die_area xMax] [$die_area yMax]"
    # debug "pad_limits: $xMin $yMin $xMax $yMax"
    set_footprint_offsets [list [expr $xMin - [$die_area xMin]] [expr $yMin - [$die_area yMin]] [expr [$die_area xMax] - $xMax] [expr [$die_area yMax] - $yMax]]

    foreach side {bottom right top left} {
      switch $side {
        "bottom" {
          # Sort by x co-ordinate : lowest to highest
          set order [lsort -command inst_compare_x $pads($side)]
        }
        "right" {
          # Sort by y co-ordinate : lowest to highest
          set order [lsort -command inst_compare_y $pads($side)]
        }
        "top" {
          # Sort by x co-ordinate : highest to lowest
          set order [lsort -decreasing -command inst_compare_x $pads($side)]
        }
        "left" {
          # Sort by y co-ordinate : highest to lowest
          set order [lsort -decreasing -command inst_compare_y $pads($side)]
        }
      }

      # debug "Order set"
      foreach inst $order {
        set type [get_library_inst_type $inst]
        if {$type == "none"} {
          # debug "inst: $inst"
          # debug "[$inst getName] [[$inst getMaster] getName]"
          continue
        }

        set idx $index($type)
        incr index($type)
        set padcell_name "${type}_${idx}"

        set_padcell_type $padcell_name $type
        set_padcell_side_name $padcell_name $side
        set_footprint_padcell_inst $padcell_name $inst
        if {[is_padcell_signal_type $padcell_name]} {
          # debug "Signal type"
          if {[set signal_name [get_padcell_design_signal_name $padcell_name]] != ""} {
            set_padcell_signal_name $padcell_name [get_padcell_design_signal_name $padcell_name]
          }
        } else {
          # debug "Non-signal type: $padcell_name [$inst getName]"
          if {![is_padcell_physical_only $padcell_name]} {
            set_padcell_signal_name $padcell_name [$inst getName]
          }
        }
      }
    }
    # debug "end"

    check_footprint
  }

  proc get_power_nets {} {
    variable footprint
    variable block

    set power_nets {}
    if {![dict exists $footprint power_nets]} {
      foreach net [$block getNets] {
        if {[$net getSigType] == "POWER"} {
          lappend power_nets [$net getName]
        }
      }

      dict set footprint power_nets $power_nets
    }
    return [dict get $footprint power_nets]
  }

  proc get_ground_nets {} {
    variable footprint
    variable block

    set ground_nets {}
    if {![dict exists $footprint ground_nets]} {
      foreach net [$block getNets] {
        if {[$net getSigType] == "GROUND"} {
          lappend ground_nets [$net getName]
        }
      }

      dict set footprint ground_nets $ground_nets
    }
    return [dict get $footprint ground_nets]
  }

  proc write_signal_mapping {signal_map_file} {
    variable footprint

    # debug "start"
    if {[catch {set ch [open $signal_map_file "w"]} msg]} {
       utl::error 44 "Cannot open file $signal_map_file."
    }

    foreach padcell [dict keys [dict get $footprint padcell]] {
      if {[is_padcell_physical_only $padcell]} {continue}
      puts $ch "$padcell [get_padcell_signal_name $padcell]"
    }

    close $ch
  }

  proc write_footprint {footprint_file} {
    variable footprint

    # debug "start"
    if {[catch {set ch [open $footprint_file "w"]} msg]} {
      utl::error 45 "Cannot open file $footprint_file."
    }

    puts $ch "source \$::env(FOOTPRINT_LIBRARY)"
    puts $ch ""
    puts $ch "Footprint definition \{"
    puts $ch "  Type wirebond"
    puts $ch ""
    # debug "die area: [get_die_area]"

    set die_area [get_die_area]
    puts $ch "  die_area \"$die_area\""
    if {[is_footprint_defined scaled_core_area]} {
      # debug "core area [get_scaled_core_area]"
      set scaled_core_area [get_scaled_core_area]
      # debug [llength $scaled_core_area]
      set core_area {}
      foreach value $scaled_core_area {
        lappend core_area [ord::dbu_to_microns $value]
      }
      puts $ch "  core_area \"$core_area\""
    }
    # debug "scaled_offsets [get_footprint_offsets]"
    set scaled_offsets [get_footprint_offsets]
    set offsets {}
    foreach value $scaled_offsets {
      lappend offsets [ord::dbu_to_microns $value]
    }

    # debug "pad_layer: [get_footprint_pad_pin_layer]"
    puts $ch "  offsets \"$offsets\""
    puts $ch "  pin_layer \"[get_footprint_pad_pin_layer]\""
    puts $ch "  pad_pin_name \"%s\""
    puts $ch "  pad_inst_name \"%s\""
    # debug "power nets: [get_power_nets]"
    if {[llength [set power_nets [get_power_nets]]] > 0} {
      # debug "power_nets $power_nets"
      puts $ch "  power_nets \"$power_nets\""
    } else {
      utl::warn "PAD" 46 "No power nets found in design."
    }
    if {[llength [set ground_nets [get_ground_nets]]] > 0} {
      puts $ch "  ground_nets \"$ground_nets\""
    } else {
      utl::warn "PAD" 47 "No ground nets found in design."
    }
    if {[dict exists $footprint place]} {
      puts $ch "  place \{"
      dict for {name placement} [dict get $footprint place] {
        puts -nonewline $ch "    $name \{"
        if {[dict exists $placement type]} {
          puts -nonewline "type [dict get $placement type]"
        }
        if {[dict exists $placement name]} {
          puts -nonewline "name [dict get $placement name]"
        }
        if {[dict exists $placement cell]} {
          set origin [get_origin $name]
          puts -nonewline "cell {origin {x [dict get $origin x] y [dict get $origin y]} orient [get_orient $name]}"
        }
        puts $ch "\}"
      }
      puts $ch "  \}"
    }

    # debug "padcell"
    if {[dict exists $footprint padcell]} {
      puts $ch "  padcell \{"
      foreach padcell_name [dict keys [dict get $footprint padcell]] {
        set origin [get_padcell_origin $padcell_name]
        puts -nonewline $ch "    $padcell_name \{type [get_padcell_type $padcell_name]"
        # debug "origin: $origin"
        puts -nonewline $ch " cell {origin {x [dict get $origin x] y [dict get $origin y]} orient [get_padcell_orient $padcell_name]}"
        if {[has_padcell_bondpad $padcell_name]} {
          set bondpad_origin [get_padcell_bondpad_origin $padcell_name]
          puts -nonewline $ch " bondpad {origin {x [dict get $bondpad_origin x] y [dict get $bondpad_origin y]} orient [get_padcell_bondpad_orient $padcell_name]}"
        }
        puts $ch "\}"
      }
      puts $ch "  \}"
    }
    puts $ch "\}"

    close $ch
  }

  proc load_footprint {footprint_file} {
    source $footprint_file

    initialize
  }

  proc init_process_footprint {} {
    variable footprint
    variable db
    variable tech
    variable block
    variable chip_width
    variable chip_height


    set db [::ord::get_db]
    set tech [$db getTech]
    set block [[$db getChip] getBlock]

    if {![dict exists $footprint die_area]} {
      if {[catch {set_die_area {*}[ord::get_die_area]} msg]} {
        utl::error PAD 223 "Design data must be loaded before this command."
      }
    }

    set chip_width  [get_footprint_die_size_x]
    set chip_height [get_footprint_die_size_y]

    init_offsets

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
  }

  variable checks {}
  proc consistency_check {name attribute value} {
    variable checks

    if {[dict exists $checks $attribute $value]} {
      utl::error PAD 217 "Attribute $attribute $value for padcell $name has already been used for padcell [dict get $checks $attribute $value]."
    }
  }

  proc register_check {name attribute value} {
    variable checks

    dict set checks $attribute $value $name
  }

  proc check_footprint {} {
    variable footprint
    variable checks

    if {[is_footprint_wirebond]} {
      init_library_bondpad
    } elseif {[is_footprint_flipchip]} {
      init_rdl
    }

    set checks {}
    foreach padcell_name [dict keys [dict get $footprint padcell]] {
      # debug "Checking $padcell, data: [dict get $footprint padcell $padcell]"
      set padcell [dict get $footprint padcell $padcell_name]
      dict set padcell name $padcell_name
      # debug "$padcell_name: $padcell"
      set padcell [verify_padcell $padcell]

      dict set footprint padcell $padcell_name $padcell
    }

    # Lookup side assignment
    foreach padcell [dict keys [dict get $footprint padcell]] {
      # debug [dict get $footprint padcell $padcell]
      set side_name [get_padcell_side_name $padcell]
      dict set footprint padcells $side_name $padcell [dict get $footprint padcell $padcell]
    }

    if {[dict exists $footprint place]} {
      foreach place_name [dict keys [dict get $footprint place]] {
        set cell [dict get $footprint place $place_name]
        dict set cell name $place_name
        set cell [verify_cell_inst $cell]

        place_cell \
          -inst_name [dict get $cell inst_name] \
          -cell [dict get $cell cell_name] \
          -origin [list [ord::dbu_to_microns [dict get $cell cell scaled_origin x]] [ord::dbu_to_microns [dict get $cell cell scaled_origin y]]] \
          -orient [dict get $cell cell orient] \
          -status "FIRM"
      }
    }
  }

  proc get_padcell_pad_pin_name {padcell} {
    variable tech

    return [get_library_pad_pin_name [get_padcell_type $padcell]]
  }

  proc get_library_pad_pin_shape {padcell} {
    variable tech
    # debug "$padcell"

    set inst [get_padcell_inst $padcell]
    # debug "[$inst getName]"
    # debug "[[$inst getMaster] getName]"
    # debug "[get_padcell_type $padcell]"

    set mterm [[$inst getMaster] findMTerm [get_padcell_pad_pin_name $padcell]]
    foreach  mpin  [$mterm getMPins] {

      foreach geometry [$mpin getGeometry] {
        if {[[$geometry getTechLayer] getName] == [get_footprint_pad_pin_layer]} {
          set pin_box [list [$geometry xMin] [$geometry yMin] [$geometry xMax] [$geometry yMax]]
          return $pin_box
        }
      }
    }
  }

  proc get_padcell_pad_pin_shape {padcell} {
    set inst [get_padcell_inst $padcell]
    set pin_box [pdngen::transform_box {*}[get_library_pad_pin_shape $padcell] [$inst getOrigin] [$inst getOrient]]
    return $pin_box
  }

  proc get_box_center {box} {
    return [list [expr ([lindex $box 2] + [lindex $box 0]) / 2] [expr ([lindex $box 3] + [lindex $box 1]) / 2]]
  }

  proc has_padcell_signal_name {padcell} {
    variable footprint

    return [dict exists $footprint padcell $padcell use_signal_name]
  }

  proc add_physical_pin {padcell inst} {
    variable block
    variable tech

    if {![has_padcell_signal_name $padcell]} {
      return
    }

    set term [$block findBTerm [get_padcell_signal_name $padcell]]
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
        utl::warn "PAD" 20 "Cannot find pin [get_library_pad_pin_name [get_padcell_type $padcell]] on cell [[$inst getMaster] getName]."
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
          utl::warn "PAD" 19 "Cannot find shape on layer [get_footprint_pad_pin_layer] for [$inst getName]:[[$inst getMaster] getName]:[$mterm getName]."
          return 0
        }
      }
    }
    if {[get_padcell_type $padcell] == "sig"} {
      utl::warn "PAD" 4 "Cannot find a terminal [get_padcell_signal_name $padcell] for ${padcell}."
    }
  }

  proc connect_to_bondpad_or_bump {inst center padcell} {
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

    set term [$block findBTerm [get_padcell_signal_name $padcell]]
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
            utl::info "PAD" 51 "Creating padring net: $assigned_name."
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
        utl::info "PAD" 52 "Creating padring net: _UNASSIGNED_$idx."
        set net [odb::dbNet_create $block "_UNASSIGNED_$idx"]
        set term [odb::dbBTerm_create $net "_UNASSIGNED_$idx"]
      } else {
        utl::warn "PAD" 12 "Cannot find a terminal [get_padcell_signal_name $padcell] for $padcell to associate with bondpad [$inst getName]."
        return
      }
    }

    # debug "padcell: $padcell, net: $net"
    $net setSpecial
    $net setSigType $type

    set pin [odb::dbBPin_create $term]
    set layer [$tech findLayer [get_footprint_pad_pin_layer]]
    if {$layer == "NULL"} {
      utl::error PAD 78 "Layer [get_footprint_pad_pin_layer] not defined in technology."
    }
    set x [dict get $center x]
    set y [dict get $center y]

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
          set origin [get_padcell_bondpad_scaled_origin $padcell]
          # debug "padcell, $padcell, signal_name: $signal_name"

          set inst [odb::dbInst_create $block $cell "bp_${signal_name}"]
          # debug "inst $inst, block: $block, cell: $cell"

          # debug "Add bondpad to padcell: $padcell"
          $inst setOrigin [dict get $origin x] [dict get $origin y]
          $inst setOrient [get_padcell_orient $padcell bondpad]
          $inst setPlacementStatus "FIRM"

          set center [get_padcell_scaled_center $padcell bondpad]
          connect_to_bondpad_or_bump $inst $center $padcell
        } else {
          if {[set inst [get_padcell_inst $padcell]] == "NULL"} {
            utl::warn "PAD" 48 "No padcell instance found for $padcell."
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
    variable library

    set die_size_x [get_footprint_die_size_x]
    set die_size_y [get_footprint_die_size_y]
    set pitch [get_footprint_bump_pitch]
    set bump_width [get_footprint_bump_width]
    # debug "$pitch $bump_width"

    if {[dict exists $library bump array_size]} {
      set num_bumps_x [dict get $library bump array_size columns]
      set num_bumps_y [dict get $library bump array_size rows]

      if {[dict exists $library bump offset]} {
        set actual_tile_offset_x [expr [ord::microns_to_dbu [dict get $library bump offset x]] - $pitch / 2]
        set actual_tile_offset_y [expr [ord::microns_to_dbu [dict get $library bump offset y]] - $pitch / 2]
      } else {
        set actual_tile_offset_x [expr ($die_size_x - $num_bumps_x * $pitch) / 2]
        set actual_tile_offset_y [expr ($die_size_y - $num_bumps_y * $pitch) / 2]
      }
    } else {
      if {[dict exists $library bump offset]} {
        set actual_tile_offset_x [expr [ord::microns_to_dbu [dict get $library bump offset x]] - $pitch / 2]
        set actual_tile_offset_y [expr [ord::microns_to_dbu [dict get $library bump offset y]] - $pitch / 2]
        set available_bump_space_x [expr $die_size_x - $actual_tile_offset_x]
        set available_bump_space_y [expr $die_size_y - $actual_tile_offset_y]
      } else {
        set min_bump_spacing_to_die_edge [get_footprint_min_bump_spacing_to_die_edge]
        set tile_spacing_to_die_edge [expr $min_bump_spacing_to_die_edge - ($pitch - $bump_width) / 2]
        set available_bump_space_x [expr $die_size_x - 2 * $tile_spacing_to_die_edge]
        set available_bump_space_y [expr $die_size_y - 2 * $tile_spacing_to_die_edge]
      }
      set num_bumps_x [expr int(1.0 * $available_bump_space_x / $pitch)]
      set num_bumps_y [expr int(1.0 * $available_bump_space_y / $pitch)]

      if {![dict exists $library bump offset]} {
        set actual_tile_offset_x [expr ($die_size_x - $num_bumps_x * $pitch) / 2]
        set actual_tile_offset_y [expr ($die_size_y - $num_bumps_y * $pitch) / 2]
      }
    }

    # debug "tile_spacing_to_die_edge $tile_spacing_to_die_edge"

    # debug "available_bump_space_x $available_bump_space_x"
    # debug "available_bump_space_y $available_bump_space_y"

    # debug "num_bumps_x $num_bumps_x"
    # debug "num_bumps_y $num_bumps_y"
  }

  proc get_bump_pitch_table {} {
    variable library

    if {![dict exists $library scaled lookup_by_pitch]} {
      if {[dict exists $library lookup_by_pitch]} {
        dict for {key value} [dict get $library lookup_by_pitch] {
          set scaled_key [ord::microns_to_dbu $key]
          dict set library scaled lookup_by_pitch $scaled_key $value
        }
      } else {
        utl::error "PAD" 34 "No bump pitch table defined in the library."
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

    if {![dict exists $footprint scaled bump_pitch]} {
      if {[dict exists $footprint bump pitch]} {
        dict set footprint scaled bump_pitch [ord::microns_to_dbu [dict get $footprint bump pitch]]
      } else {
        dict set footprint scaled bump_pitch [get_library_bump_pitch]
      }
    }
    return [dict get $footprint scaled bump_pitch]
  }

  proc get_footprint_bump_width {} {
    variable footprint

    if {![dict exists $footprint scaled bump_width]} {
      if {[dict exists $footprint bump width]} {
        dict set footprint scaled bump_width [ord::microns_to_dbu [dict get $footprint bump width]]
      } else {
        dict set footprint scaled bump_width [get_library_bump_width]
      }
    }
    return [dict get $footprint scaled bump_width]
  }

  proc get_footprint_rdl_width {} {
    variable footprint

    if {![dict exists $footprint scaled rdl_width]} {
      if {[dict exists $footprint rdl width]} {
        dict set footprint scaled rdl_width [ord::microns_to_dbu [dict get $footprint rdl width]]
      } else {
        dict set footprint scaled rdl_width [get_library_rdl_width]
      }
    }

    return [dict get $footprint scaled rdl_width]
  }

  proc get_footprint_rdl_spacing {} {
    variable footprint

    if {![dict exists $footprint scaled rdl_spacing]} {
      if {[dict exists $footprint rdl spacing]} {
        dict set footprint scaled rdl_spacing [ord::microns_to_dbu [dict get $footprint rdl spacing]]
      } else {
        dict set footprint scaled rdl_spacing [get_library_rdl_spacing]
      }
    }

    return [dict get $footprint scaled rdl_spacing]
  }

  proc get_library_bump_pitch {} {
    variable library

    if {![dict exists $library scaled bump_pitch]} {
      if {[dict exists $library bump pitch]} {
        dict set library scaled bump_pitch [ord::microns_to_dbu [dict get $library bump pitch]]
      } else {
        utl::error "PAD" 35 "No bump_pitch defined in library data."
      }
    }
    return [dict get $library scaled bump_pitch]
  }

  proc get_library_bump_width {} {
    variable library
    variable db

    if {![dict exists $library scaled bump_width]} {
      if {[dict exists $library bump width]} {
        dict set library scaled bump_width [ord::microns_to_dbu [dict get $library bump width]]
      } else {
        if {[dict exists $library bump cell_name]} {
          if {[llength [dict get $library bump cell_name]] == 1} {
            set cell_name [dict get $library bump cell_name]
          } else {
            set cell_name [lookup_by_bump_pitch [dict get $library bump cell_name]]
          }
          # debug "$cell_name, [$db findMaster $cell_name]"
          if {[set master [$db findMaster $cell_name]] != "NULL"} {
            dict set library scaled bump_width [$master getWidth]
            dict set library scaled bump_height [$master getHeight]
          } elseif  {[dict exists $library cells $cell_name width]} {
            dict set library scaled bump_width [ord::microns_to_dbu [dict get $library cells $cell_name width]]
          } else {
            utl::error "PAD" 36 "No width defined for selected bump cell $cell_name."
          }
        } else {
          utl::error "PAD" 37 "No bump_width defined in library data."
        }
      }
    }
    return [dict get $library scaled bump_width]
  }

  proc get_library_bump_pin_name {} {
    variable library

    if {![dict exists $library bump pin_name]} {
      utl::error "PAD" 38 "No bump_pin_name attribute found in the library."
    }
    return [dict get $library bump pin_name]
  }

  proc get_library_rdl_width {} {
    variable library

    if {![dict exists $library scaled rdl_width]} {
      if {[dict exists $library rdl width]} {
        dict set library scaled rdl_width [ord::microns_to_dbu [dict get $library rdl width]]
      } else {
        utl::error "PAD" 39 "No rdl_width defined in library data."
      }
    }
    return [dict get $library scaled rdl_width]
  }

  proc get_library_rdl_spacing {} {
    variable library

    if {![dict exists $library scaled rdl_spacing]} {
      if {[dict exists $library rdl spacing]} {
        dict set library scaled rdl_spacing [ord::microns_to_dbu [dict get $library rdl spacing]]
      } else {
        utl::error "PAD" 40 "No rdl_spacing defined in library data."
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

    if {![dict exists $library rdl layer_name]} {
      dict set library rdl layer_name [lindex $pdngen::metal_layers end]
    }

    return [dict get $library rdl layer_name]
  }

  proc get_footprint_pads_per_pitch {} {
    variable footprint
    if {![dict exists $footprint pads_per_pitch]} {
      utl::error PAD 79 "Footprint does not have the pads_per_pitch attribute specified."
    }
    return [dict get $footprint pads_per_pitch]
  }

  proc get_footprint_corner_size {} {
    variable library

    if {![dict exists $library num_pads_per_tile]} {
      utl::error PAD 162 "Required setting for num_pads_per_tile not found."
    }

    if {[llength [dict get $library num_pads_per_tile]] > 1} {
      set pads_per_pitch [lookup_by_bump_pitch [dict get $library num_pads_per_tile]]
    } else {
      set pads_per_pitch [dict get $library num_pads_per_tile]
    }

    return $pads_per_pitch
  }

  proc is_power_net {net_name} {
    variable footprint

    if {[lsearch [get_power_nets] $net_name] > -1} {
      return 1
    }

    return 0
  }

  proc is_ground_net {net_name} {
    variable footprint

    if {[lsearch [get_ground_nets] $net_name] > -1} {
      return 1
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
      default {utl::error "PAD" 27 "Illegal orientation $orientation specified."}
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
      default {utl::error "PAD" 28 "Illegal orientation $orientation specified."}
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

        if {[set padcell [get_padcell_at_row_col $row $col]] == ""} {continue}
        if {[is_padcell_unassigned $padcell]} {continue}

        set side [get_padcell_side_name $padcell]

        switch $side {
          "bottom" {
            set orientation "R0"
            set tile_origin [list [expr $actual_tile_offset_x + ($col - 1) * $tile_width] [lindex $die_area 1]]
            set trace_func "path_trace_[expr $num_bumps_y - $row]"
          }
          "right" {
            set orientation "R90"
            set tile_origin [list [lindex $die_area 2] [expr $actual_tile_offset_y + ($num_bumps_y - $row) * $tile_width]]
            set trace_func "path_trace_[expr $num_bumps_x - $col]"
          }
          "top" {
            set orientation "R180"
            set tile_origin [list [expr $actual_tile_offset_x + $col * $tile_width] [lindex $die_area 3]]
            set trace_func "path_trace_[expr $row - 1]"
          }
          "left" {
            set orientation "R270"
            set tile_origin [list [lindex $die_area 0] [expr $actual_tile_offset_y + ($num_bumps_y - $row + 1) * $tile_width]]
            set trace_func "path_trace_[expr $col - 1]"
          }
        }

        set padcell_pin_center [get_box_center [get_padcell_pad_pin_shape $padcell]]
        set path [transform_path [$trace_func {*}[invert_transform {*}$padcell_pin_center $tile_origin $orientation]] $tile_origin $orientation]

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

        if {[dict exists $traces [get_padcell_signal_name $padcell]]} {
          dict set traces [get_padcell_signal_name $padcell] [concat [dict get $traces [get_padcell_signal_name $padcell]] $padcell]
        } else {
          dict set traces [get_padcell_signal_name $padcell] $padcell
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

  variable bumps {}
  proc bump_exists {row col} {
    variable bumps

    if {[dict exists $bumps $row $col removed]} {
      return 0
    }
    return 1
  }

  proc bump_remove {row col} {
    variable bumps

    dict set bumps $row $col removed 1
  }

  proc bump_set_net_name {row col net_name} {
    variable bumps
    variable footprint

    if {[dict exists $bumps $row $col net]} {
      if {[dict get $bumps $row $col net] != $net_name} {
        utl::error PAD 238 "Trying to set bump at ($row $col) to be $net_name, but it has already been set to [dict get $bumps $row $col net]."
      }
    }
    dict set bumps $row $col net $net_name
  }

  proc bump_set_net {row col net_name} {
    variable bumps

    check_net_type $net_name signal
    bump_set_net_name $row $col $net_name
  }

  proc bump_get_net {row col} {
    variable bumps

    if {[dict exists $bumps $row $col net]} {
      return [dict get $bumps $row $col net]
    }
    return ""
  }

  proc check_net_type {net_name type} {
    variable bumps

    if {[dict exists $bumps nets $net_name]} {
      if {[dict get $bumps nets $net_name] != $type} {
        utl::error PAD 235 "Net $net_name specified as a $type net, but has alreaqdy been defined as a [dict get $bumps nets $net_name] net."
      }
    }

    dict set bumps nets $net_name $type
  }

  proc bump_set_power {row col power_net} {
    variable bumps

    check_net_type $power_net power
    bump_set_net_name $row $col $power_net
    add_power_nets $power_net
    dict set bumps $row $col power 1
  }

  proc bump_is_power {row col} {
    variable bumps

    if {[dict exists $bumps $row $col power]} {
      return [dict get $bumps $row $col power]
    }
    return 0
  }

  proc bump_set_ground {row col ground_net} {
    variable bumps

    check_net_type $ground_net ground
    bump_set_net_name $row $col $ground_net
    add_ground_nets $ground_net
    dict set bumps $row $col ground 1
  }

  proc bump_is_ground {row col} {
    variable bumps

    if {[dict exists $bumps $row $col ground]} {
      return [dict get $bumps $row $col ground]
    }
    return 0
  }

  proc place_bumps {} {
    variable num_bumps_x
    variable num_bumps_y
    variable block

    set corner_size [get_footprint_corner_size]
    set bump_master [get_cell bump]
    lassign [$bump_master getOrigin] master_x master_y

    for {set row 1} {$row <= $num_bumps_y} {incr row} {
      for {set col 1} {$col <= $num_bumps_x} {incr col} {
        if {![bump_exists $row $col]} {continue}
        set origin [get_bump_origin $row $col]
        set bump_name [get_bump_name_at_row_col $row $col]

        set inst [odb::dbInst_create $block $bump_master $bump_name]

        $inst setOrigin [expr [dict get $origin x] + $master_x] [expr [dict get $origin y] + $master_y]
        $inst setOrient "R0"
        $inst setPlacementStatus "FIRM"

        set padcell [get_padcell_at_row_col $row $col]
        if {$padcell != ""} {
          connect_to_bondpad_or_bump $inst [get_bump_center $row $col] $padcell
        }
      }
    }
    # debug "end"
  }

  proc bump_get_tag {row col} {
    set net_name [bump_get_net $row $col]
    if {[bump_is_power $row $col]} {
      if {$net_name == "VDD"} {
        return "POWER"
      } else {
        return "POWER_[bump_get_net $row $col]"
      }
    } elseif {[bump_is_ground $row $col]} {
      if {$net_name == "VSS"} {
        return "GROUND"
      } else {
        return "GROUND_[bump_get_net $row $col]"
      }
    }
    utl::error PAD 236 "Bump $row $col is not assigned to power or ground."
  }

  proc add_power_ground_rdl_straps {} {
    variable num_bumps_x
    variable num_bumps_y
    variable tech

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

    set rdl_min_spacing [[$tech findLayer $rdl_routing_layer] getSpacing]
    set power_nets {}
    set ground_nets {}
    # Add stripes for bumps in the central core area
    if {[pdngen::get_dir $rdl_routing_layer] == "hor"} {
      for {set row [expr $corner_size + 1]} {$row <= $num_bumps_y - $corner_size} {incr row} {
        for {set col [expr $corner_size + 1]} {$col <= $num_bumps_x - $corner_size} {incr col} {
          if {[bump_get_net $row $col] == ""} {
            if {$row % 2 == 0} {
              bump_set_power $row $col "VDD"
            } else {
              bump_set_ground $row $col "VSS"
            }
          }
        }
      }
      # debug "minX: [ord::dbu_to_microns $minX], maxX: [ord::dbu_to_microns $maxX]"
      for {set row [expr $corner_size + 1]} {$row <= $num_bumps_y - $corner_size} {incr row} {
        set prev_tag ""
        set y [dict get [get_bump_center $row 1] y]
        set lowerY [expr $y - ($bump_pitch - $rdl_stripe_width - $rdl_stripe_spacing) / 2]
        set upperY [expr $y + ($bump_pitch - $rdl_stripe_width - $rdl_stripe_spacing) / 2]
        for {set col [expr $corner_size + 1]} {$col <= $num_bumps_x - $corner_size} {incr col} {
          if {![bump_exists $row $col]} {
            set prev_tag ""
            continue
          }
          set point [get_bump_center $row $col]
          set net_name [bump_get_net $row $col]
          set tag [bump_get_tag $row $col]
          if {[bump_is_power $row $col]} {
            lappend power_nets $net_name
          } elseif {[bump_is_ground $row $col]} {
            lappend ground_nets $net_name
          }
          # debug $net_name
          if {$prev_tag == ""} {
            set minX [expr [dict get $point x] - $bump_pitch / 2]
          } elseif {$prev_tag == $tag} {
            set minX [expr [dict get $point x] - $bump_pitch / 2 - $rdl_min_spacing / 2]
          } else {
            set minX [expr [dict get $point x] - $bump_pitch / 2 + $rdl_min_spacing / 2]
          }
          set prev_tag $tag
          if {$col == $num_bumps_x - $corner_size} {
            set maxX [expr [dict get $point x] + $bump_pitch / 2]
          } else {
            set maxX [expr [dict get $point x] + $bump_pitch / 2 - $rdl_min_spacing / 2]
          }

          set upper_stripe [odb::newSetFromRect $minX [expr $upperY - $rdl_stripe_width / 2] $maxX [expr $upperY + $rdl_stripe_width / 2]]
          set lower_stripe [odb::newSetFromRect $minX [expr $lowerY - $rdl_stripe_width / 2] $maxX [expr $lowerY + $rdl_stripe_width / 2]]
          pdngen::add_stripe $rdl_routing_layer $tag $upper_stripe
          pdngen::add_stripe $rdl_routing_layer $tag $lower_stripe
          set link_stripe [odb::newSetFromRect [expr [dict get $point x] - $rdl_stripe_width / 2] $lowerY [expr [dict get $point x] + $rdl_stripe_width / 2] $upperY]
          pdngen::add_stripe $rdl_routing_layer $tag $link_stripe
        }
      }
    } elseif {[pdngen::get_dir $rdl_routing_layer] == "ver"} {
      # Columns numbered top to bottom, so maxY is for corner_size +1

      # debug "minY: [ord::dbu_to_microns $minY], maxY: [ord::dbu_to_microns $maxY]"
      for {set row [expr $corner_size + 1]} {$row <= $num_bumps_y - $corner_size} {incr row} {
        for {set col [expr $corner_size + 1]} {$col <= $num_bumps_x - $corner_size} {incr col} {
          if {[bump_get_net $row $col] == ""} {
            if {$col % 2 == 0} {
              bump_set_power $row $col "VDD"
            } else {
              bump_set_ground $row $col "VSS"
            }
          }
        }
      }
      for {set col [expr $corner_size + 1]} {$col <= $num_bumps_x - $corner_size} {incr col} {
        set prev_tag ""
        set x [dict get [get_bump_center 1 $col] x]
        set lowerX [expr $x - ($bump_pitch - $rdl_stripe_width - $rdl_stripe_spacing) / 2]
        set upperX [expr $x + ($bump_pitch - $rdl_stripe_width - $rdl_stripe_spacing) / 2]
        for {set row [expr $corner_size + 1]} {$row <= $num_bumps_y - $corner_size} {incr row} {
          if {![bump_exists $row $col]} {
            set prev_tag ""
            continue
          }
          set net_name [bump_get_net $row $col]
          set tag [bump_get_tag $row $col]
          if {[bump_is_power $row $col]} {
            lappend power_nets $net_name
          } elseif {[bump_is_ground $row $col]} {
            lappend ground_nets $net_name
          }
          set point [get_bump_center $row $col]
          # debug $tag
          if {$prev_tag == ""} {
            set maxY [expr [dict get $point y] + $bump_pitch / 2]
          } elseif {$prev_tag == $tag} {
            set maxY [expr [dict get $point y] + $bump_pitch / 2 + $rdl_min_spacing / 2]
          } else {
            set maxY [expr [dict get $point y] + $bump_pitch / 2 - $rdl_min_spacing / 2]
          }
          if {$row == $num_bumps_y - $corner_size} {
            set minY [expr [dict get $point y] - $bump_pitch / 2]
          } else {
            set minY [expr [dict get $point y] - $bump_pitch / 2 + $rdl_min_spacing / 2]
          }
          # debug "row: $row, col: $col, x: $x, y: [dict get $point y], prev: $prev_tag, tag: $tag, minY: $minY, maxY, $maxY"
          set prev_tag $tag
          set upper_stripe [odb::newSetFromRect [expr $upperX - $rdl_stripe_width / 2] $minY [expr $upperX + $rdl_stripe_width / 2] $maxY]
          set lower_stripe [odb::newSetFromRect [expr $lowerX - $rdl_stripe_width / 2] $minY [expr $lowerX + $rdl_stripe_width / 2] $maxY]
          pdngen::add_stripe $rdl_routing_layer $tag $upper_stripe
          pdngen::add_stripe $rdl_routing_layer $tag $lower_stripe
          set link_stripe [odb::newSetFromRect $lowerX [expr [dict get $point y] - $rdl_stripe_width / 2] $upperX [expr [dict get $point y] + $rdl_stripe_width / 2]]
          pdngen::add_stripe $rdl_routing_layer $tag $link_stripe
        }
      }
    }

    # debug "$pdngen::metal_layers"
    # debug "[array get pdngen::stripe_locs]"
    pdngen::merge_stripes
    dict set pdngen::design_data power_nets [lsort -unique $power_nets]
    dict set pdngen::design_data ground_nets [lsort -unique $ground_nets]
    dict set pdngen::design_data core_domain "CORE"
    pdngen::opendb_update_grid
  }
    # Connect up pads in the corner regions
    # Connect group of padcells up to the column of allocated bumps
    
  proc get_side_anchor_points {side_name} {
    set anchor_points ""
    foreach padcell [get_footprint_padcells_by_side $side_name] {
      if {[has_padcell_location $padcell]} {
        lappend anchor_points $padcell
	#debug "$padcell is an anchor point"
      }
    }
    return $anchor_points 
  }
   
  proc has_padcell_location {padcell} {
    variable footprint
    return  [dict exists $footprint padcell $padcell cell center] || [dict exists $footprint padcell $padcell cell scaled_center] || \
            [dict exists $footprint padcell $padcell cell origin] || [dict exists $footprint padcell $padcell cell scaled_origin]
  }


  proc get_padcell_center {padcell {type cell}} {
    variable footprint

    if {![dict exists $footprint padcell $padcell $type center]} {
       utl::error PAD 231 "No center information specified for $inst_name."
    }   
    return [dict get $footprint padcell $padcell $type center]
  }  
  
  proc set_padcell_origin {padcell center} {
    variable footprint
    set side_name [get_padcell_side_name $padcell]
    set orient [get_padcell_orient $padcell]
    set name [get_padcell_inst_name $padcell]
    set type [get_padcell_type $padcell]
    set cell [get_cell $type $side_name]
    set cell_height [ord::dbu_to_microns [expr max([$cell getHeight],[$cell getWidth])]]
    set cell_width  [ord::dbu_to_microns [expr min([$cell getHeight],[$cell getWidth])]]

    #debug "padcell: $padcell , center: $center , width: $cell_width , height: $cell_height , orient: $orient"  
    set origin  [get_origin $center $cell_width $cell_height $orient]
    dict set footprint padcell $padcell cell origin $origin
    return  $origin
  }
  
  proc set_padcell_scaled_origin {padcell} {
    variable footprint   
    dict set footprint padcell $padcell cell scaled_origin [get_scaled_origin $padcell]
  }  
     
  proc get_padcell_width {padcell} {
    set side_name [get_padcell_side_name $padcell]
    set type [get_padcell_type $padcell]
    set cell [get_cell $type $side_name]
    return [expr min([$cell getHeight],[$cell getWidth])]
  }
   
  proc get_padcell_height {padcell} {
    set side_name [get_padcell_side_name $padcell]
    set type [get_padcell_type $padcell]
    set cell [get_cell $type $side_name]
    return [expr max([$cell getHeight],[$cell getWidth])]
  } 
  
  proc assign_locations {} {
    variable footprint
    variable pad_ring     
    foreach side_name {bottom right top left} {
      switch $side_name \
        "bottom" {
	  set anchor_corner_a corner_ll
          set anchor_corner_b corner_lr
        } \
        "right"  {
	  set anchor_corner_a corner_lr
          set anchor_corner_b corner_ur        
	} \
        "top"    {
	  set anchor_corner_a corner_ur
          set anchor_corner_b corner_ul        
        } \
        "left"   {
	  set anchor_corner_a corner_ul
          set anchor_corner_b corner_ll
	}
	   
      set unplaced_pads {}
      set anchor_cell_a $anchor_corner_a
      set anchor_cell_b $anchor_corner_b
      foreach padcell [get_footprint_padcells_by_side $side_name] {
	if {![has_padcell_location $padcell]} {
          lappend unplaced_pads $padcell
        } else {	
	  set anchor_cell_b $padcell
          add_locations $side_name  $unplaced_pads $anchor_cell_a $anchor_cell_b 
	  set unplaced_pads {}
	  set anchor_cell_a $anchor_cell_b	  
	}
      }
      add_locations $side_name  $unplaced_pads $anchor_cell_a $anchor_corner_b
      #debug "side_name: $side_name , anchor_cells: $anchor_cell_a $anchor_cell_b , unplaced_pads: $unplaced_pads"  
    }  
  }
  
  proc add_locations {side_name  unplaced_pads anchor_cell_a anchor_cell_b } {
    variable pad_ring     
    variable chip_width 
    variable chip_height
    variable edge_bottom_offset 
    variable edge_right_offset 
    variable edge_top_offset 
    variable edge_left_offset
        
    switch $side_name \
      "bottom" {
        if {[regexp corner_ $anchor_cell_a ]} {
          set inst [dict get $pad_ring $anchor_cell_a]
	  set start [[$inst getBBox] xMax]           
        } else {     
          set cell_width_a [get_padcell_width $anchor_cell_a]
          set start [expr [dict get [get_scaled_center $anchor_cell_a] x] + $cell_width_a / 2]
        } 
        if {[regexp corner_ $anchor_cell_b]} {
          set inst [dict get $pad_ring $anchor_cell_b]            
	  set end [[$inst getBBox] xMin]           
        } else {     
          set cell_width_b [get_padcell_width $anchor_cell_b]
          set end [expr [dict get [get_scaled_center $anchor_cell_b] x] - $cell_width_b / 2]
        } 
      } \
      "right"  {
        if {[regexp corner_ $anchor_cell_a]} {
          set inst [dict get $pad_ring $anchor_cell_a]
	  set start [[$inst getBBox] yMax]           
        } else {     
          set cell_width_a [get_padcell_width $anchor_cell_a]
          set start [expr [dict get [get_scaled_center $anchor_cell_a] y] + $cell_width_a / 2]
        } 
        if {[regexp corner_ $anchor_cell_b]} {
          set inst [dict get $pad_ring $anchor_cell_b]            
	  set end [[$inst getBBox] yMin]           
        } else {     
          set cell_width_b [get_padcell_width $anchor_cell_b]
          set end [expr [dict get [get_scaled_center $anchor_cell_b] y] - $cell_width_b / 2]
        } 
      } \
      "top"    {
        if {[regexp corner_ $anchor_cell_a]} {
          set inst [dict get $pad_ring $anchor_cell_a]
	  set start [[$inst getBBox] xMin]           
        } else {     
          set cell_width_a [get_padcell_width $anchor_cell_a]
          set start [expr [dict get [get_scaled_center $anchor_cell_a] x] + $cell_width_a / 2]
        } 
        if {[regexp corner_ $anchor_cell_b]} {
          set inst [dict get $pad_ring $anchor_cell_b]            
	  set end [[$inst getBBox] xMax]           
        } else {     
          set cell_width_b [get_padcell_width $anchor_cell_b]
          set end [expr [dict get [get_scaled_center $anchor_cell_b] x] - $cell_width_b / 2]
        } 
      } \
      "left"   {
      
        if {[regexp corner_ $anchor_cell_a]} {
          set inst [dict get $pad_ring $anchor_cell_a]
	  set start [[$inst getBBox] yMin]           
        } else {     
          set cell_width_a [get_padcell_width $anchor_cell_a]
          set start [expr [dict get [get_scaled_center $anchor_cell_a] y] + $cell_width_a / 2]
        } 
        if {[regexp corner_ $anchor_cell_b]} {
          set inst [dict get $pad_ring $anchor_cell_b]            
	  set end [[$inst getBBox] yMax]           
        } else {     
          set cell_width_b [get_padcell_width $anchor_cell_b]
          set end [expr [dict get [get_scaled_center $anchor_cell_b] y] - $cell_width_b / 2]
        }
      } 

    set sideCount [llength $unplaced_pads]
    set sidePadWidth 0
    set sideWidth [ expr abs($end - $start) ]
    foreach padcell $unplaced_pads {
      set sidePadWidth [expr $sidePadWidth +  [get_padcell_width $padcell]]
    }	   
    set siteStart $start
    set gridSnap 1000
    set PadSpacing [expr  $sideWidth / (1 + $sideCount) / $gridSnap * $gridSnap ]
    # debug "side $side_name has $sideCount PADs , with PadSpacing: $PadSpacing, sideWidth $sideWidth sidePadWidth $sidePadWidth $fill_start $fill_end" 
    # debug "$side_name: segment starts with $anchor_cell_a  and ends with $anchor_cell_b , start is: $start , and end is $end"
    if [expr ($sideWidth - $sidePadWidth) < 0] {utl::error PAD 232 "Cannot fit IO pads between the following anchor cells : $anchor_cell_a, $anchor_cell_b."}
    foreach padcell $unplaced_pads {
      set padOrder [ expr 1 + [lsearch $unplaced_pads $padcell]] 

      switch $side_name \
    	"bottom" {
    	  set pad_center_x [expr $siteStart + ($PadSpacing * $padOrder) ] 
    	  set pad_center_y [expr $edge_bottom_offset + round (0.5 * [get_padcell_height $padcell]) ]
    	} \
    	"right"  {
    	  set pad_center_x [expr $chip_width - $edge_right_offset - round (0.5 * [get_padcell_height $padcell]) ]
    	  set pad_center_y [expr $siteStart + ($PadSpacing * $padOrder) ] 
    	} \
    	"top" {
    	  set pad_center_x [expr $siteStart - ($PadSpacing * $padOrder) ] 
    	  set pad_center_y [expr $chip_height - $edge_bottom_offset - round (0.5 * [get_padcell_height $padcell])  ]
    	} \
    	"left" {
    	  set pad_center_x [expr $edge_left_offset + round (0.5 * [get_padcell_height $padcell])]
    	  set pad_center_y [expr $siteStart - ($PadSpacing * $padOrder) ]
    	}
      #debug "padOrder: $padOrder pad_center_x: $pad_center_x  pad_center_y: $pad_center_y"

      set pad_center_x_microns [ord::dbu_to_microns $pad_center_x]
      set pad_center_y_microns [ord::dbu_to_microns $pad_center_y]
      set_padcell_center $padcell $pad_center_x_microns $pad_center_y_microns
      set_padcell_scaled_center $padcell $pad_center_x $pad_center_y	    
      set_padcell_origin $padcell [get_padcell_center $padcell]     
      set_padcell_scaled_origin $padcell       
      #debug  " pad_cell has origin: [get_padcell_origin $padcell] ,  scaled_origin: [get_padcell_scaled_origin $padcell] , center: [get_padcell_center $padcell]] , scaled_center [get_padcell_scaled_center $padcell]]"
    }        
  } 
   
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
        utl::error "PAD" 15 "No cells found on $side_name side."
      }
      foreach padcell [get_footprint_padcells_by_side $side_name] {
        set name [get_padcell_inst_name $padcell]
        set type [get_padcell_type $padcell]
        set cell [get_cell $type $side_name]
        # debug "name: $name, type: $type, cell: $cell"
	
        set cell_height [expr max([$cell getHeight],[$cell getWidth])]
        set cell_width  [expr min([$cell getHeight],[$cell getWidth])]
		
        if {[set inst [get_padcell_inst $padcell]] == "NULL"} {
          # debug "Skip padcell $padcell"
          continue
        }

        set orient [get_padcell_orient $padcell]
        set origin [get_padcell_scaled_origin $padcell]
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

    if {![dict exists $pad_ring $corner]} {
      utl::error PAD 80 "Attribute $corner not specified in pad_ring ($pad_ring)."
    }
    set inst [dict get $pad_ring $corner]
    return [[$inst getBBox] getDX]
  }

  proc corner_height {corner} {
    variable pad_ring

    if {![dict exists $pad_ring $corner]} {
      utl::error PAD 81 "Attribute $corner not specified in pad_ring ($pad_ring)."
    }
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

    # debug "End"
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
        if {![dict exists $library types $type]} {
          utl::error PAD 82 "Type $type not specified in the set of library types."
        }
        set cell_ref [dict get $library types $type]
        if {![dict exists $library cells $cell_ref]} {
          utl::error PAD 83 "Cell $cell_ref of Type $type is not specified in the list of cells in the library."
        }

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
              set pin_name $signal
              if {[dict exists $library cells $cell_ref connect $signal]} {
                set pin_name [dict get $library cells $cell_ref connect $signal]
              }
              # debug "Adding inst_name $name pin_name $pin_name"
              # Dont add breakers into netlist, as they are physical_only
              # lappend pad_segment($signal,[dict get $segment $signal cur_index]) [list inst_name $name pin_name $pin_name]
            }
          }
        } else {
          foreach signal [dict get $library connect_by_abutment] {
            if {$type == "fill"} {continue}
            if {$type == "corner"} {continue}
            set pin_name $signal
            if {[dict exists $library cells $cell_ref connect $signal]} {
              set pin_name [dict get $library cells $cell_ref connect $signal]
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
      set nets_created {}
      dict for {signal sections} [dict get $segment cells] {
        # debug "Signal: $signal"
        set section_keys [dict keys $sections]
        foreach section $section_keys {
          # debug "Section: $section"

          # Determine net name for ring
          # search for existing nets
          set candidate_nets [dict create]
          foreach inst_pin_name [dict get $sections $section] {
            set inst_name [dict get $inst_pin_name inst_name]
            set pin_name [dict get $inst_pin_name pin_name]

            if {[set inst [$block findInst $inst_name]] == "NULL"} {
              # debug "Cant find instance $inst_name"
              continue
            }
            if {[set iterm [$inst findITerm $pin_name]] == "NULL"} {
              continue
            }
            if {[set net [$iterm getNet]] == "NULL"} {
              # no net connected so we'll assume we need to make it
              continue
            }
            dict set candidate_nets $net ""
          }
          set candidate_nets [dict keys $candidate_nets]
          set net "NULL"
          if {[llength $candidate_nets] == 1} {
            set net [lindex $candidate_nets 0]
          } elseif {[llength $candidate_nets] > 1} {
            utl::warn PAD 50 "Multiple nets found on $signal in padring."
          }

          if {$net == "NULL"} {
            # no net was found, we need to make it
            set net_name "${signal}"
            if {[llength $section_keys] > 1} {
              append net_name "_$section"
            }
            if {[set net [$block findNet $net_name]] != "NULL"} {
              utl::error "PAD" 14 "Net ${signal}_$section already exists, so cannot be used in the padring."
            } else {
              lappend nets_created $net_name
              set net [odb::dbNet_create $block $net_name]
            }
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
              utl::warn "PAD" 18 "No terminal $signal found on $inst_name."
            }
          }
        }
      }
      if {[llength $nets_created] > 0} {
        utl::info "PAD" 53 "Creating padring nets: [join $nets_created {, }]."
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
      if {![dict exists $library types $type]} {
        utl::error PAD 84 "Type $type not specified in the set of library types."
      }
      set cell_ref [dict get $library types $type]
      if {![dict exists $library cells $cell_ref]} {
        utl::error PAD 85 "Cell $cell_ref of Type $type is not specified in the list of cells in the library."
      }
      set library_cell [dict get $library cells $cell_ref]

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
      utl::error 29 "No types specified in the library."
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

  proc verify_libcell_setup {} {
    variable library

    if {[dict exists $library types]} {
      set required_types {fill corner}
      if {[is_footprint_flipchip]} {
        lappend required_types bump
      }

      foreach required_type $required_types {
        if {![dict exists $library types $required_type]} {
          utl::error PAD 207 "Required type of cell ($required_type) has no libcell definition."
        }
      }
    }

    foreach type [dict keys [dict get $library types]] {
      foreach cell [dict get $library types $type] {
        if {[dict exists $library cells $cell]} {
          set cell_ref [dict get $library cells $cell]
          dict set cell_ref type $type
          dict set library cells $cell $cell_ref
        }
      }
    }

    if {[dict exists $library cells]} {
      dict for {cell_ref_name cell_ref} [dict get $library cells] {
        if {![dict exists $cell_ref name]} {
          dict set cell_ref name $cell_ref_name
        }
        dict set library cells $cell_ref_name [verify_cell_ref $cell_ref]
      }
    }
  }

  proc init_footprint {args} {
    set arglist $args

    verify_libcell_setup

    # debug "start: $args"
    if {[set idx [lsearch $arglist "-signal_mapping"]] > -1} {
      set_signal_assignment_file [lindex $arglist [expr $idx + 1]]
      set arglist [lreplace $arglist $idx [expr $idx + 1]]
    }

    if {[llength $arglist] > 1} {
      utl::error "PAD" 30 "Unrecognized arguments to init_footprint $arglist."
    }

    # debug "arglist: $arglist"
    if {[llength $arglist] == 1} {
      set_signal_assignment_file $arglist
    }

    if {[is_footprint_flipchip]} {
      verify_bump_options
    }

    initialize
    pdngen::init_tech

    # Perform signal assignment
    # debug "assign_signals"
    assign_signals

    check_footprint

    # Padring placement
    # debug "padring placement"
    place_padcells
    place_corners
    assign_locations
    get_footprint_padcells_order
    place_padring
    connect_by_abutment

    # Wirebond pad / Flipchip bump connections
    if {[is_footprint_wirebond]} {
      place_bondpads
      # Bondpads are assumed to connect by abutment
    } elseif {[is_footprint_flipchip]} {
      # assign_padcells_to_bumps
      place_bumps
      connect_bumps_to_padcells
      add_power_ground_rdl_straps
      write_rdl_trace_def
    }
    global_assignments
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

  proc get_padcell_cell_ref {padcell} {
    variable library

    set type [get_padcell_type $padcell]
    if {![dict exists $library types $type]} {
      utl::error PAD 86 "Type $type not specified in the set of library types."
    }
    set cell_ref [dict get $library types $type]
    if {![dict exists $library cells $cell_ref]} {
      utl::error PAD 87 "Cell $cell_ref of Type $type is not specified in the list of cells in the library."
    }
    return $cell_ref
  }

  proc is_padcell_physical_only {padcell} {
    variable library

    set cell_ref [get_padcell_cell_ref $padcell]
    if {[dict exists $library cells $cell_ref physical_only]} {
      return [dict get $library cells $cell_ref physical_only]
    }

    return 0
  }

  proc is_padcell_control {padcell} {
    variable library

    set cell_ref [get_padcell_cell_ref $padcell]
    if {[dict exists $library cells $cell_ref is_control]} {
      return [dict get $library cells $cell_ref is_control]
    }

    return 0
  }

  proc get_library_types_of_cells {} {
    variable library

    if {![dict exists $library types_of_cells]} {

      dict for {type cell_ref} [dict get $library types] {
        if {[dict exists $library cells $cell_ref]} {
          if {[dict exists $library cells $cell_ref cell_name]} {
            set cell_name [dict get $library cells $cell_ref cell_name]
            if {[llength $cell_name] > 1} {
              dict for {side cell_master} [dict get $library cells $cell_ref cell_name] {
                dict set types_of_cells $cell_master $type
              }
            } else {
              dict set types_of_cells $cell_name $type
            }
          } else {
            dict set types_of_cells $cell_ref $type
          }
        } else {
          dict set types_of_cells $cell_ref $type
        }
      }
      dict set library types_of_cells $types_of_cells
    }

   return [dict get $library types_of_cells]
  }

  proc get_library_cell_masters {} {
    variable library

    return [dict keys [get_library_types_of_cells]]
  }

  proc get_library_inst_type {inst} {
    variable library
    set cell_types [get_library_types_of_cells]
    # debug "cell_types: $cell_types"

    if {[dict exists $cell_types [[$inst getMaster] getName]]} {
      return [dict get $cell_types [[$inst getMaster] getName]]
    }

    return "none"
  }

  proc is_inst_padcell {inst} {
    variable library

    set cell_masters [get_library_cell_masters]

    if {[lsearch $cell_masters [[$inst getMaster] getName] > -1} {
      return 1
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

  proc set_footprint_offsets {offset} {
    variable footprint

    dict set footprint offsets $offset
    init_offsets
  }

  proc get_footprint_offsets {} {
    variable footprint

    if {![dict exists $footprint offsets]} {
      dict set footprint offsets 0
    }

    return [dict get $footprint offsets]
  }

  proc init_offsets {} {
    variable footprint
    variable edge_bottom_offset
    variable edge_right_offset
    variable edge_top_offset
    variable edge_left_offset

    set args [get_footprint_offsets]

    if {[llength $args] == 1} {
      set edge_bottom_offset [ord::microns_to_dbu $args]
      set edge_right_offset  [ord::microns_to_dbu $args]
      set edge_top_offset    [ord::microns_to_dbu $args]
      set edge_left_offset   [ord::microns_to_dbu $args]
    } elseif {[llength $args] == 2} {
      set edge_bottom_offset [ord::microns_to_dbu [lindex $args 0]]
      set edge_right_offset  [ord::microns_to_dbu [lindex $args 1]]
      set edge_top_offset    [ord::microns_to_dbu [lindex $args 0]]
      set edge_left_offset   [ord::microns_to_dbu [lindex $args 1]]
    } elseif {[llength $args] == 4} {
      set edge_bottom_offset [ord::microns_to_dbu [lindex $args 0]]
      set edge_right_offset  [ord::microns_to_dbu [lindex $args 1]]
      set edge_top_offset    [ord::microns_to_dbu [lindex $args 2]]
      set edge_left_offset   [ord::microns_to_dbu [lindex $args 3]]
    } else {
      utl::error "PAD" 9 "Expected 1, 2 or 4 offset values, got [llength $args]."
    }

    # debug "bottom: $edge_bottom_offset, right: $edge_right_offset, top: $edge_top_offset, left: $edge_left_offset"
  }

  proc set_inner_offset {args} {
    variable db
    variable inner_bottom_offset
    variable inner_right_offset
    variable inner_top_offset
    variable inner_left_offset

    if {[llength $args] == 1} {
      set inner_bottom_offset [ord::microns_to_dbu $args]
      set inner_right_offset  [ord::microns_to_dbu $args]
      set inner_top_offset    [ord::microns_to_dbu $args]
      set inner_left_offset   [ord::microns_to_dbu $args]
    } elseif {[llength $args] == 2} {
      set inner_bottom_offset [ord::microns_to_dbu [lindex $args 0]]
      set inner_right_offset  [ord::microns_to_dbu [lindex $args 1]]
      set inner_top_offset    [ord::microns_to_dbu [lindex $args 0]]
      set inner_left_offset   [ord::microns_to_dbu [lindex $args 1]]
    } elseif {[llength $inner_offset] == 4} {
      set inner_bottom_offset [ord::microns_to_dbu [lindex $args 0]]
      set inner_right_offset  [ord::microns_to_dbu [lindex $args 1]]
      set inner_top_offset    [ord::microns_to_dbu [lindex $args 2]]
      set inner_left_offset   [ord::microns_to_dbu [lindex $args 3]]
    } else {
      utl::error "PAD" 10 "Expected 1, 2 or 4 inner_offset values, got [llength $args]."
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
    foreach cell_ref [dict keys $filler_cells] {
      set cell [dict get $filler_cells $cell_ref master]
      set width [expr min([$cell getWidth],[$cell getHeight])]
      dict set filler_cells $cell_ref width $width
      dict set spacers $width $cell_ref
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

  proc find_padcell_with_signal_name {signal_name} {
    variable footprint

    if {[is_power_net $signal_name] || [is_ground_net $signal_name]} {
      return ""
    }

    if {[dict exists $footprint padcell]} {
      foreach padcell [dict keys [dict get $footprint padcell]] {
        if {[dict exists $footprint padcell $padcell signal_name]} {
          if {[dict get $footprint padcell $padcell signal_name] == $signal_name} {
            return $padcell
          }
        }
      }
    }
    return ""
  }

  proc check_signal_name {padcell} {
    variable footprint
    variable block
    # debug "start"

    set signal_name [dict get $padcell signal_name]
    if {[$block findNet $signal_name] != "NULL"} {
      dict set padcell use_signal_name $signal_name
      # debug "end: signal found"
      return $padcell
    }

    if {[is_power_net $signal_name]} {
      dict set padcell use_signal_name $signal_name
      # debug "end: power net found"
      return $padcell
    }

    if {[is_ground_net $signal_name]} {
      dict set padcell use_signal_name $signal_name
      # debug "end: ground net found"
      return $padcell
    }

    if {$signal_name == "UNASSIGNED"} {
      dict set padcell use_signal_name $signal_name
      # debug "end: UNASSIGNED net found"
      return $padcell
    }

    if {[is_padcell_physical_only $padcell]} {
      # dict set padcell use_signal_name ""
      # debug "end: physical only padcell"
      return $padcell
    }

    if {[set signal [$block findNet $signal_name]] == "NULL"} {
      set alt_signal_name 0
      if {[dict exists $padcell pad_pin_name]} {
        set alt_signal_name 1
        set try_signal [format [dict get $padcell pad_pin_name] $signal_name]
        set signal [$block findNet $try_signal]
      } elseif {[dict exists $footprint pad_pin_name]} {
        set alt_signal_name 1
        set try_signal [format [dict get $footprint pad_pin_name] $signal_name]
        set signal [$block findNet $try_signal]
      }

      if {$signal == "NULL"} {
        if {[$block findInst $signal_name] == "NULL"} {
          if {$alt_signal_name == 1} {
            utl::error PAD 92 "No signal $signal_name or $try_signal defined for padcell."
          } else {
            utl::error PAD 91 "No signal $signal_name defined for padcell."
          }
        } else {
          dict set padcell use_signal_name $signal_name
          # debug "end: inst name as signal name padcell"
          return $padcell
        }
      } else {
        dict set padcell use_signal_name $try_signal
        # debug "end: alt signal name padcell"
        return $padcell
      }
    }

    utl::error PAD 93 "Signal \"$signal_name\" not found in design."
  }

  proc check_edge_name {edge_name} {
    set edge_names {
      top {top TOP Top T t}
      bottom {bottom BOTTOM Bottom B b}
      right {right RIGHT Right R r}
      left {left LEFT Left L l}
    }

    dict for {edge options} $edge_names {
      if {[lsearch $options $edge_name] > -1} {
        return $edge
      }
    }
    utl::error PAD 94 "Value for -edge_name ($edge_name) not permitted, choose one of bottom, right, top or left."
  }

  proc check_cell_type {type} {
    variable library

    if {[dict exists $library types]} {
      if {[dict exists $library types $type]} {
        return $type
      } else {
        utl::error PAD 95 "Value for -type ($type) does not match any library types ([dict keys [dict get $library types]])."
      }
    } else {
      utl::error PAD 98 "No library types defined."
    }
  }

  proc check_coordinate {xy} {
    if {[llength $xy] != 2} {
      utl::error PAD 239 "expecting a 2 element list in the form \"number number\"."
    }
    set coord [list x [lindex $xy 0] y [lindex $xy 1]]
    if {[catch {check_xy $coord} msg]} {
      utl::error PAD 240 "Invalid coordinate specified $msg."
    }
    return $coord
  }

  proc check_rows_columns {rc} {
    # debug $xy
    if {![dict exists $rc rows]} {
      error "no value specified for rows"
    }
    if {![dict exists $rc columns]} {
      error "no value specified for columns"
    }
    if {![is_number [dict get $rc rows]]} {
      error "rows ([dict get $rc rows]), not recognized as a nummber"
    }
    if {![is_number [dict get $rc columns]]} {
      error "columns ([dict get $rc columns]), not recognized as a nummber"
    }
  }


  proc check_array_size {rc} {
    if {[llength $rc] != 2} {
      utl::error PAD 241 "expecting a 2 element list in the form \"number number\"."
    }
    set rc_spec [list rows [lindex $rc 0] columns [lindex $rc 1]]
    if {[catch {check_rows_columns $rc_spec} msg]} {
      utl::error PAD 242 "Invalid array_size specified $msg."
    }
    return $rc_spec
  }

  proc check_xy {xy} {
    # debug $xy
    if {[llength $xy] != 4} {
      error "expecting a 4 element list in the form \"x number y number\""
    }
    if {![dict exists $xy x]} {
      error "no value specified for x"
    }
    if {![dict exists $xy y]} {
      error "no value specified for y"
    }
    if {![is_number [dict get $xy x]]} {
      error "x co-ordinate ([dict get $xy x]), not recognized as a nummber"
    }
    if {![is_number [dict get $xy y]]} {
      error "y co-ordinate ([dict get $xy y]), not recognized as a nummber"
    }
  }

  proc check_rowcol {rowcol} {
    variable num_bumps_x
    variable num_bumps_y

    if {[llength $rowcol] != 4} {
      utl::error PAD 243 "expecting a 4 element list in the form \"row <integer> col <integer>\"."
    }
    if {![regexp {[0-9]+} [dict get $rowcol row]]} {
      utl::error PAD 244 "row value ([dict get $rowcol row]), not recognized as an integer."
    }
    if {![regexp {[0-9]+} [dict get $rowcol col]]} {
      utl::error PAD 245 "col value ([dict get $rowcol col]), not recognized as an integer."
    }

    set row [dict get $rowcol row]
    set col [dict get $rowcol col]

    if {$row < 1 || $row > $num_bumps_y} {
      utl::error PAD 229 "The value for row is $row, but must be in the range 1 - $num_bumps_y."
    }
    if {$col < 1 || $col > $num_bumps_x} {
      utl::error PAD 230 "The value for col is $col, but must be in the range 1 - $num_bumps_x."
    }
  }

  proc check_orient {orient} {
    set valid {R0 R90 R180 R270 MX MY MXR90 MYR90}

    if {[lsearch $valid $orient] > -1} {
      return $orient
    }
    utl::error PAD 99 "Invalid orientation $orient, must be one of \"$valid\"."
  }

  proc check_location {location} {
    # Allowed options are:
    # {(center|origin) {x <num> y <num>} [orient (R0|R90|R180|R270|MX|MY|MXR90|MYR90)]}
    if {[llength $location] % 2 == 1} {
      utl::error PAD 100 "Incorrect number of arguments for location, expected an even number, got [llength $location] ($location)."
    }
    if {[dict exists $location center] && [dict exists $location origin]} {
      utl::error PAD 101 "Only one of center or origin may be specified for -location ($location)."
    }
    if {[dict exists $location center]} {
      if {[catch {check_xy [dict get $location center]} msg]} {
        # debug "center msg: $msg"
        utl::error PAD 102 "Incorrect value specified for -location center ([dict get $location center]), $msg."
      }
    } elseif {[dict exists $location origin]} {
      if {[catch {check_xy [dict get $location origin]} msg]} {
        # debug "origin msg: $msg"
        utl::error PAD 103 "Incorrect value specified for -location origin ([dict get $location origin]), $msg."
      }
    } else {
      utl::error PAD 104 "Required origin or center not specified for -location ($location)."
    }

    if {[dict exists $location orient]} {
      check_orient [dict get $location orient]
    }

    return $location
  }

  proc check_bondpad {location} {
    variable footprint

    if {![is_footprint_wirebond]} {
      utl::error PAD 105 "Specification of bondpads is only allowed for wirebond padring layouts."
    }
    return [check_location $location]
  }

  proc check_bump {bump} {
    variable footprint

    if {![is_footprint_flipchip]} {
      utl::error PAD 106 "Specification of bumps is only allowed for flipchip padring layouts."
    }

    check_rowcol $bump

    return $bump
  }

  proc check_inst_name {padcell} {
    variable block
    variable footprint

    if {[dict exists $padcell inst_name]} {
      set inst_name [dict get $padcell inst_name]
    } else {
      if {[dict exists $padcell use_signal_name] && ([is_power_net [dict get $padcell use_signal_name]] || [is_ground_net [dict get $padcell use_signal_name]])} {
        if {[dict exists $footprint pad_inst_name]} {
          set inst_name [format [dict get $footprint pad_inst_name] [dict get $padcell name]]
        } else {
          set inst_name [dict get $padcell name]
        }
      } elseif {[set inst_name [check_inst_name_versus_signal $padcell]] == "NULL"} {
        if {[dict exists $footprint pad_inst_name]} {
          if {[dict exists $padcell use_signal_name]} {
            set inst_name [format [dict get $footprint pad_inst_name] [dict get $padcell use_signal_name]]
          } else {
            set inst_name [format [dict get $footprint pad_inst_name] [dict get $padcell name]]
          }
        } else {
          if {[dict exists $padcell use_signal_name]} {
            set inst_name [dict get $padcell use_signal_name]
          } else {
            set inst_name [dict get $padcell name]
          }
        }
      }
    }

    if {[set inst [$block findInst $inst_name]] == "NULL"} {
      set cell [get_padcell_cell_name $padcell]
      if {[is_padcell_physical_only $padcell]} {
        # debug "Create physical_only cell $cell with name $inst_name for padcell $padcell"
        # debug "master: [get_cell_master $cell]"
        set inst [odb::dbInst_create $block [get_cell_master $cell] $inst_name]
        dict set padcell inst $inst
        # debug "done"
      } elseif {[is_padcell_control $padcell]} {
        # debug "Create control cell $cell with name $inst_name for padcell $padcell"
        set inst [odb::dbInst_create $block [get_cell_master $cell] $inst_name]
        dict set padcell inst $inst
      } elseif {[is_footprint_create_padcells]} {
        # debug "Create cell $cell with name $inst_name for padcell $padcell"
        set inst [odb::dbInst_create $block [get_cell_master $cell] $inst_name]
        dict set padcell inst $inst
      } elseif {[dict exists $padcell use_signal_name] && ([is_power_net [dict get $padcell use_signal_name]] || [is_ground_net [dict get $padcell use_signal_name]])} {
        # debug "Create power/ground $cell with name $inst_name for padcell $padcell"
        set inst [odb::dbInst_create $block [get_cell_master $cell] $inst_name]
        dict set padcell inst $inst
      } else {
        utl::error PAD 122 "Cannot find an instance with name \"$inst_name\"."
      }
    }
    dict set padcell inst_name [$inst getName]
    dict set padcell inst $inst

    return $padcell
  }

  proc check_inst_name_versus_signal {padcell} {
    variable block

    # If signal name is specified, check this signal is connected to the correct pin of the specified instance
    if {[dict exists $padcell use_signal_name]} {
      set signal_name [dict get $padcell use_signal_name]
      if {[set net [$block findNet $signal_name]] != "NULL"} {
        set net [$block findNet $signal_name]
        if {$net != "NULL"} {
          set pad_pin_name [get_padcell_pad_pin_name $padcell]
          # debug "Found net [$net getName] for $padcell"
          set found_pin 0
          foreach iTerm [$net getITerms] {
            # debug "Connection [[$iTerm getInst] getName] ([[$iTerm getMTerm] getName]) for $padcell with signal $signal_name"
            if {[[$iTerm getMTerm] getName] == $pad_pin_name} {
              # debug "Found instance [[$iTerm getInst] getName] for $padcell with signal $signal_name"
              dict set $padcell inst [$iTerm getInst]
              return [[$iTerm getInst] getName]
            }
          }
        }
      }
    }

    return "NULL"
  }

  proc check_cell_name {cell_name} {
    variable library
    variable db

    if {[dict exists $library cells] && [dict exists $library cells $cell_name]} {
      return $cell_name
    } else {
      get_cell_master $cell_name
    }

    return $cell_name
  }

  variable type_index {}

  proc is_number {value} {
    return [regexp {[\+\-]?[0-9]*[\.]?[0-9]*} $value]
  }

  proc set_die_area {args} {
    variable footprint

    if {[llength $args] == 1} {
      if {[llength [lindex $args 0]] == 4} {
        set arglist [lindex $args 0]
      } else {
        utl::error PAD 142 "Unexpected number of arguments for set_die_area."
      }
    } elseif {[llength $args] == 4} {
      set arglist $args
    } else {
      utl::error PAD 143 "Unexpected number of arguments for set_die_area."
    }

    set die_area {}
    foreach value $arglist {
      if {[is_number $value]} {
        lappend die_area $value
      }
    }

    dict set footprint die_area $die_area
  }

  proc set_core_area {args} {
    variable footprint

    if {[llength $args] == 1} {
      if {[llength [lindex $args 0]] == 4} {
        set arglist [lindex $args 0]
      } else {
        utl::error PAD 144 "Unexpected number of arguments for set_core_area."
      }
    } elseif {[llength $args] == 4} {
      set arglist $args
    } else {
      utl::error PAD 145 "Unexpected number of arguments for set_core_area."
    }

    set core_area {}
    foreach value $arglist {
      if {[is_number $value]} {
        lappend core_area $value
      }
    }

    dict set footprint core_area $core_area
  }

  proc add_power_nets {args} {
    variable footprint

    foreach arg $args {
      if {[dict exists $footprint power_nets]} {
        if {[lsearch [dict get $footprint power_nets] $arg] == -1} {
          dict lappend footprint power_nets $arg
        }
      } else {
        dict lappend footprint power_nets $arg
      }
    }
  }

  proc add_ground_nets {args} {
    variable footprint

    foreach arg $args {
      if {[dict exists $footprint ground_nets]} {
        if {[lsearch [dict get $footprint ground_nets] $arg] == -1} {
          dict lappend footprint ground_nets $arg
        }
      } else {
        dict lappend footprint ground_nets $arg
      }
    }
  }

  proc set_offsets {value} {
    variable footprint

    if {[llength $value] == 1} {
      if {[is_number $value]} {
        set_footprint_offsets $value
      }
    } elseif {[llength $value] == 2 || [llength $value] == 4} {
      foreach number $value {
        is_number $number
      }
      set_footprint_offsets $value
    } else {
      utl::error PAD 205 "Incorrect number of values specified for offsets ([llength $value]), expected 1, 2 or 4."
    }
  }

  proc set_pin_layer {layer_name} {
    variable footprint

    set tech [ord::get_db_tech]
    dict set footprint pin_layer [check_layer_name $layer_name]
  }

  proc set_pad_inst_name {format_string} {
    variable footprint

    if {[catch {format $format_string test} msg]} {
      utl::error PAD 147 "The pad_inst_name value must be a format string with exactly one string substitution %s."
    }
    dict set footprint pad_inst_name $format_string
  }

  proc set_pad_pin_name {format_string} {
    variable footprint

    if {[catch {format $format_string test} msg]} {
      utl::error PAD 160 "The pad_pin_name value must be a format string with exactly one string substitution %s."
    }
    dict set footprint pad_pin_name $format_string
  }

  proc set_rdl_cover_file_name {file_name} {
    variable footprint

    dict set footprint rdl_cover_file_name $file_name
  }

  proc check_layer_name {layer_name} {
    set tech [ord::get_db_tech]
    if {[$tech findLayer $layer_name] == "NULL"} {
      utl::error PAD 146 "Layer $layer_name is not a valid layer for this technology."
    }
    return $layer_name
  }

  proc set_bump_options {args} {
    variable library

    set process_args $args
    while {[llength $process_args] > 0} {
      set arg [lindex $process_args 0]
      set value [lindex $process_args 1]

      switch $arg {
        -pitch               {dict set library bump pitch $value}
        -bump_pin_name       {dict set library bump bump_pin_name $value}
        -spacing_to_edge     {dict set library bump spacing_to_edge $value}
        -offset              {dict set library bump offset [check_coordinate $value]}
        -array_size          {dict set library bump array_size [check_array_size $value]}
        -cell_name           {dict set library bump cell_name $value}
        -num_pads_per_tile   {dict set library num_pads_per_tile [check_num_pads_per_tile_option $value]}
        -rdl_layer           {dict set library rdl layer_name [check_layer_name $value]}
        -rdl_width           {dict set library rdl width $value}
        -rdl_spacing         {dict set library rdl spacing $value}
        -rdl_cover_file_name {set_rdl_cover_file_name $value}
        default {utl::error PAD 111 "Unrecognized argument $arg, should be one of -pitch, -bump_pin_name, -spacing_to_edge, -cell_name, -bumps_per_tile, -rdl_layer, -rdl_width, -rdl_spacing."}
      }

      set process_args [lrange $process_args 2 end]
    }
  }

  proc check_num_pads_per_tile_option {num_pads_per_tile} {
    if {[llength $num_pads_per_tile] == 1} {
      if {$num_pads_per_tile < 1 || $num_pads_per_tile > 5} {
        utl::error PAD 209 "The number of padcells within a pad pitch ($num_pads_per_tile) must be a number between 1 and 5."
      }
    } else {
      dict for {pitch value} $num_pads_per_tile {
        if {$value < 1 || $value > 5} {
          utl::error PAD 210 "The number of padcells within a pad pitch (pitch $pitch: num_padcells: $value) must be a number between 1 and 5."
        }
      }
    }
    return $num_pads_per_tile
  }

  proc verify_bump_options {} {
    variable library
    set tech [ord::get_db_tech]

    if {[dict exists $library rdl layer_name]} {
    } else {
      utl::error PAD 211 "No RDL layer specified."
    }
    set rdl_layer_name [dict get $library rdl layer_name]
    set rdl_layer [$tech findLayer $rdl_layer_name]

    if {[dict exists $library rdl width]} {
      set scaled_rdl_width [ord::microns_to_dbu [dict get $library rdl width]]
      set min_width [$rdl_layer getMinWidth]
      set max_width [$rdl_layer getMaxWidth]

      if {$scaled_rdl_width < $min_width} {
        utl::error PAD 212 "Width set for RDL layer $rdl_layer_name ([ord::dbu_to_microns $scaled_rdl_width]), is less than the minimum width of the layer in this technology ([ord::dbu_to_microns $min_width])."
      }
      if {$scaled_rdl_width > $max_width} {
        utl::error PAD 213 "Width set for RDL layer $rdl_layer_name ([ord::dbu_to_microns $scaled_rdl_width]), is greater than the maximum width of the layer in this technology ([ord::dbu_to_microns $max_width])."
      }
    } else {
      dict set library rdl width [$rdl_layer getWidth]
    }
    set rdl_width [dict get $library rdl width]

    if {[dict exists $library rdl spacing]} {
      set scaled_rdl_spacing [ord::microns_to_dbu [dict get $library rdl spacing]]
      set spacing [$rdl_layer getSpacing]

      if {$scaled_rdl_spacing < $spacing} {
        utl::error PAD 214 "Spacing set for RDL layer $rdl_layer_name ([ord::dbu_to_microns $scaled_rdl_spacing]), is less than the required spacing for the layer in this technology ([ord::dbu_to_microns $spacing])."
      }
    }

    if {[dict exists $library num_pads_per_tile]} {
      check_num_pads_per_tile_option [dict get $library num_pads_per_tile]
    } else {
      utl::error PAD 215 "The number of pads within a bump pitch has not been specified."
    }
  }

  proc add_pad {args} {
    variable footprint
    variable block
    variable type_index
    variable db
    variable tech

    set db [::ord::get_db]
    set tech [$db getTech]
    set block [ord::get_db_block]

    set padcell_name {}

    if {[llength $args] % 2 == 1} {
        utl::error PAD 109 "Incorrect number of arguments for add_pad - expected an even number, received [llength $args]."
    }

    set padcell {}
    # Determine the padcell name
    if {[dict exists $args -name]} {
      set padcell_name [dict get $args -name]
    } else {
      if {[dict exists $args -type]} {
        set type [dict get $args -type]
        if {![dict exists $type_index $type]} {
          dict set type_index $type 0
        }
        set idx [dict get $type_index $type]
        set padcell_name "${type}_$idx"
        incr idx
        dict set type_index $type $idx
      } else {
        utl::error PAD 110 "Must specify -type option if -name is not specified."
      }
    }
    if {[dict exists $footprint padcell $padcell_name]} {
      utl::error PAD 216 "A padcell with the name $padcell_name already exists."
    }
    dict set padcell name $padcell_name

    set process_args $args
    while {[llength $process_args] > 0} {
      set arg [lindex $process_args 0]
      set value [lindex $process_args 1]

      switch $arg {
        -name      {dict set padcell name $value}
        -signal    {dict set padcell signal_name $value}
        -edge      {dict set padcell side [check_edge_name $value]}
        -type      {dict set padcell type [check_cell_type $value]}
        -cell      {dict set padcell cell_name [check_cell_name $value]}
        -location  {dict set padcell cell [check_location $value]}
        -bump      {dict set padcell bump [check_bump $value]}
        -bondpad   {dict set padcell bondpad [check_bondpad $value]}
        -inst_name {dict set padcell inst_name $value}
        default {utl::error PAD 200 "Unrecognized argument $arg, should be one of -name, -signal, -edge, -type, -cell, -location, -bump, -bondpad, -inst_name."}
      }

      set process_args [lrange $process_args 2 end]
    }

    if {[dict exists $args -signal]} {
      if {[set padcell_duplicate [find_padcell_with_signal_name [dict get $padcell signal_name]]] != {}} {
        utl::error PAD 112 "Padcell $padcell_duplicate already defined to use [dict get $padcell signal_name]."
      }
    }

    dict set footprint padcell $padcell_name [verify_padcell $padcell]
  }

  proc verify_padcell {padcell} {
    variable footprint
    variable library
    variable db
    variable unassigned_idx

    # debug $padcell

    if {![dict exists $padcell name]} {
      utl::error PAD 123 "Attribute 'name' not defined for padcell $padcell."
    }
    set padcell_name [dict get $padcell name]

    # Verify signal_name
    if {[dict exists $padcell signal_name]} {
      if {[dict get $padcell signal_name] == "UNASSIGNED"} {
        set idx $unassigned_idx
        incr unassigned_idx
        dict set padcell signal_name "UNASSIGNED_$idx"
      }
      set padcell [check_signal_name $padcell]
      set signal_name [dict get $padcell use_signal_name]
    }

    # Verify type
    if {[dict exists $padcell type]} {
      check_cell_type [dict get $padcell type]

      if {[dict exists $padcell cell_ref]} {
        set cell_name [dict get $padcell cell_ref]
        set expected_cell_name [dict get $library types [dict get $padcell type]]

        if {$cell_name != $expected_cell_name} {
          utl::error PAD 140 "Type [dict get $padcell type] (cell ref - $expected_cell_name) does not match specified cell_name ($cell_name) for padcell $padcell)."
        }
      } else {
        if {[dict exists $library types [dict get $padcell type]]} {
          dict set padcell cell_ref [dict get $library types [dict get $padcell type]]
          if {[llength [dict get $padcell cell_ref]] > 1} {
            utl::error PAD 124 "Cell type $type does not exist in the set of library types."
          }
        }
      }
    } elseif {[dict exists $padcell cell_ref]} {
      check_cell_ref [dict get $padcell cell_ref]
    } else {
      utl::error PAD 125 "No type specified for padcell $padcell."
    }
    set cell_ref [dict get $padcell cell_ref]

    # Verify center/origin
    if {[dict exists $padcell cell center] && [dict exists $padcell cell origin]} {
      utl::error PAD 126 "Only one of center or origin should be used to specify the location of padcell $padcell."
    }

    # Verify side
    if {![dict exists $padcell side]} {
      if {[dict exists $padcell inst] && [[dict get $padcell inst] getOrigin] != "NULL"} {
        set inst [dict get $padcell inst]
        set bbox [$inst getBBox]
        set inst_center [list [expr ([$inst_bbox xMax] + [$inst_bbox xMin]/2)] [expr ([$inst_bbox yMax] + [$inst_bbox yMin]) / 2]]
        set side_name [get_side_name {*}$inst_center]
      } elseif {[dict exists $padcell cell center]} {
        dict set padcell cell scaled_center [get_scaled_center $padcell]
        set side_name [get_side_name [dict get $padcell cell scaled_center x] [dict get $padcell cell scaled_center y]]
      } elseif {[dict exists $padcell cell origin]} {
        dict set padcell cell scaled_origin [get_scaled_origin $padcell]
        set side_name [get_side_name [dict get $padcell cell scaled_origin x] [dict get $padcell cell scaled_origin y]]
      } elseif {[dict exists $padcell cell orient]} {
        set side_name [get_side_from_orient [dict get $padcell cell_ref] [dict get $padcell cell orient]]
      } else {
        utl::error PAD 127 "Cannot determine side for padcell $padcell, need to sepecify the location or the required edge for the padcell."
      }
      dict set padcell side $side_name
      if {![dict exists $padcell cell orient]} {
        if {[dict exists $library cells [dict get $padcell cell_ref] orient $side_name]} {
          dict set padcell cell orient [dict get $library cells [dict get $padcell cell_ref] orient $side_name]
        } else {
          utl::error PAD 128 "No orientation specified for $cell_ref for side $side_name."
        }
      }

      # debug "$padcell, $side_name"
    }
    set side_name [dict get $padcell side]

    # Verify cell_name
    if {![dict exists $padcell cell_name]} {
      if {![dict exists $library cells $cell_ref]} {
        utl::info PAD 159 "Cell reference $cell_ref not found in library, setting cell_name to $cell_ref."
        set cell_name $cell_ref
      } elseif {[llength [dict get $library cells $cell_ref cell_name]] == 1} {
        set cell_name [dict get $library cells $cell_ref cell_name]
      } elseif {[dict exists $library cells $cell_ref cell_name $side_name]} {
        set cell_name [dict get $library cells $cell_ref cell_name $side_name]
      } else {
        utl::error PAD 129 "Cannot determine cell name for $padcell_name from library element $cell_ref."
      }
      dict set padcell cell_name $cell_name
    }
    set cell_name [dict get $padcell cell_name]
    if {[$db findMaster $cell_name] == "NULL"} {
      utl::error PAD 130 "Cell $cell_name not loaded into design."
    }

    # Verify inst_name
    # debug $padcell
    set padcell [check_inst_name $padcell]
    set inst_name [dict get $padcell inst_name]

    # Verify cell orientation
    if {[dict exists $padcell cell orient]} {
      set orient [dict get $padcell cell orient]
      set side_from_orient [get_side_from_orient $cell_ref [dict get $padcell cell orient]]
      if {$side_from_orient != $orient} {
        utl::error PAD 131 "Orientation of padcell $padcell_name is $orient, which is different from the orientation expected for padcells on side $side_name ($side_from_orient)."
      }
    } else {
      if {[dict exists $library cells $cell_ref orient $side_name]} {
        dict set padcell cell orient [dict get $library cells $cell_ref orient $side_name]
      } else {
        utl::error PAD 132 "Missing orientation information for $cell_ref on side $side_name."
      }
    }
    set orient [dict get $padcell cell orient]

    if {[dict exists $padcell cell]} {
      dict set padcell cell [verify_placement [dict get $padcell cell] $cell_name cell]
    }

    # Verify bondpad
    if {[dict exists $padcell bondpad]} {
      set bondpad_cell_ref [dict get $library types bondpad]
      if {![dict exists $library cells $bondpad_cell_ref]} {
        if {[$db findMaster $bondpad_cell_ref] == "NULL"} {
          utl::error PAD 133 "Bondpad cell $bondpad_cell_ref not found in library definition."
        }
      }
      if {[dict exists $padcell bondpad orient]} {
        if {[dict exists $library cells $bondpad_cell_ref orient]} {
          if {[dict exists $library cells $bondpad_cell_ref orient $side_name]} {
            set expected_orient [dict get $library cells $bondpad_cell_ref orient $side_name]
          } else {
            if {[llength [dict get $library cells $bondpad_cell_ref orient]] == 1} {
              set expected_orient [dict get $library cells $bondpad_cell_ref orient]
            } else {
              utl::error PAD 134 "Unexpected value for orient attribute in library definition for $bondpad_cell_ref."
            }
          }
          if {$expected_orient != [dict get $padcell bondpad orient]} {
            utl::warn PAD 135 "Expected orientation ($expected_orient) of bondpad for padcell $padcell_name, overridden with value [dict exists $padcell bondpad orient]."
          }
        }
      } else {
        if {[dict exists $library cells $bondpad_cell_ref orient]} {
          if {[dict exists $library cells $bondpad_cell_ref orient $side_name]} {
            dict set padcell bondpad orient [dict get $library cells $cell_ref orient $side_name]
          } else {
            if {[llength [dict get $library cells $bondpad_cell_ref orient]] == 1} {
              dict set padcell bondpad orient [dict get $library cells $cell_ref orient]
            } else {
              utl::error PAD 136 "Unexpected value for orient attribute in library definition for $bondpad_cell_ref."
            }
          }
        } else {
          utl::error PAD 137 "Missing orientation information for $cell_ref on side $side_name."
        }
      }
      set bondpad_orient [dict get $padcell bondpad orient]
      dict set padcell bondpad [verify_placement [dict get $padcell bondpad] $bondpad_cell_ref bondpad]
    }

    # Verify bump
    if {[dict exists $padcell bump]} {
      variable num_bumps_x
      variable num_bumps_y

      dict set padcell bump [check_bump [dict get $padcell bump]]

      check_rowcol [dict get $padcell bump]
      set row [dict get $padcell bump row]
      set col [dict get $padcell bump col]

      set pitch [get_footprint_bump_pitch]
      set center [get_bump_center $row $col]

      lassign [get_scaled_die_area] xMin yMin xMax yMax
      switch [dict get $padcell side] {
        "bottom" {set xMin [expr [dict get $center x] - $pitch / 2]; set xMax [expr [dict get $center x] + $pitch / 2]}
        "right"  {set yMin [expr [dict get $center y] - $pitch / 2]; set yMax [expr [dict get $center y] + $pitch / 2]}
        "top"    {set xMin [expr [dict get $center x] - $pitch / 2]; set xMax [expr [dict get $center x] + $pitch / 2]}
        "left"   {set yMin [expr [dict get $center y] - $pitch / 2]; set yMax [expr [dict get $center y] + $pitch / 2]}
      }
      if {[dict get $padcell cell scaled_center x] < $xMin || [dict get $padcell cell scaled_center x] > $xMax} {
        utl::error PAD 163 "Padcell $padcell x location ([ord::dbu_to_microns [dict get $padcell cell scaled_center x]]) cannot connect to the bump $row,$col on the $side_name edge. The x location must satisfy [ord::dbu_to_microns $xMin] <= x <= [ord::dbu_to_microns $xMax]."
      }
      if {[dict get $padcell cell scaled_center y] < $yMin || [dict get $padcell cell scaled_center y] > $yMax} {
        utl::error PAD 164 "Padcell $padcell y location ([ord::dbu_to_microns [dict get $padcell cell scaled_center y]]) cannot connect to the bump $row,$col on the $side_name edge. The y location must satisfy [ord::dbu_to_microns $yMin] <= y <= [ord::dbu_to_microns $yMax]."
      }

      if {[dict exists $padcell use_signal_name]} {
        bump_set_net_name $row $col $signal_name
      } else {
        if {[set net [bump_get_net $row $col]] != ""} {
          dict set padcell signal_name $net
          set padcell [check_signal_name $padcell]
        }
      }
    }

    if {[dict exists $padcell use_signal_name]} {
      if {![is_power_net $signal_name] && ![is_ground_net $signal_name]} {
        consistency_check $padcell_name signal [dict get $padcell use_signal_name]
      }
    }

    consistency_check $padcell_name instance $inst_name
    if {[dict exists $padcell bump]} {
      consistency_check $padcell_name row_col "row=$row, col=$col"
    }

    if {[dict exists $padcell use_signal_name]} {
      if {![is_power_net $signal_name] && ![is_ground_net $signal_name]} {
        register_check $padcell_name signal [dict get $padcell use_signal_name]
      }
    }
    register_check $padcell_name instance $inst_name
    if {[dict exists $padcell bump]} {
      register_check $padcell_name row_col "row=$row, col=$col"
    }

    # debug $padcell
    # debug [dict get $ICeWall::footprint padcell $padcell_name]
    # debug "end"
    return $padcell
  }

  proc verify_cell_inst {cell_inst} {
    variable footprint
    variable library
    variable db

    # debug $cell_inst

    # place {
    #   marker0 {type marker inst_name u_marker_0   cell {center {x 1200.000 y 1200.000} orient R0}}
    # }

    if {![dict exists $cell_inst name]} {
      utl::error PAD 165 "Attribute 'name' not defined for cell $cell_inst."
    }
    set name [dict get $cell_inst name]

    # Verify type
    if {[dict exists $cell_inst type]} {
      check_cell_type [dict get $cell_inst type]

      if {[dict exists $cell_inst cell_ref]} {
        set cell_name [dict get $cell_inst cell_ref]
        set expected_cell_name [dict get $library types [dict get $cell_inst type]]

        if {$cell_name != $expected_cell_name} {
          utl::error PAD 166 "Type [dict get $cell_inst type] (cell ref - $expected_cell_name) does not match specified cell_name ($cell_name) for cell $name)."
        }
      } else {
        if {[dict exists $library types [dict get $cell_inst type]]} {
          dict set cell_inst cell_ref [dict get $library types [dict get $cell_inst type]]
          if {[llength [dict get $cell_inst cell_ref]] > 1} {
            utl::error PAD 167 "Cell type $type does not exist in the set of library types."
          }
        }
      }
    } elseif {[dict exists $cell_inst cell_ref]} {
      check_cell_ref [dict get $cell_inst cell_ref]
    } else {
      utl::error PAD 168 "No type specified for cell $name."
    }
    set cell_ref [dict get $cell_inst cell_ref]

    # Verify center/origin
    if {[dict exists $cell_inst cell center] && [dict exists $cell_inst cell origin]} {
      utl::error PAD 169 "Only one of center or origin should be used to specify the location of cell $name."
    }

    # Verify cell_name
    if {![dict exists $cell_inst cell_name]} {
      if {![dict exists $library cells $cell_ref]} {
        set cell_name $cell_ref
      } elseif {[llength [dict get $library cells $cell_ref cell_name]] == 1} {
        set cell_name [dict get $library cells $cell_ref cell_name]
      } else {
        utl::error PAD 170 "Cannot determine library cell name for cell $name."
      }
      dict set cell_inst cell_name $cell_name
    }
    set cell_name [dict get $cell_inst cell_name]
    if {[$db findMaster $cell_name] == "NULL"} {
      utl::error PAD 171 "Cell $cell_name not loaded into design."
    }

    # Verify inst_name
    if {![dict exists $cell_inst inst_name]} {
      dict set cell_inst inst_name $name
    }

    # Verify cell orientation
    if {[dict exists $cell_inst cell orient]} {
      dict set cell_inst cell orient [check_orient [dict get $cell_inst cell orient]]
    } else {
      if {[dict exists $library cells $cell_ref orient]} {
        dict set cell_inst cell orient [dict get $library cells $cell_ref orient]
      } else {
        utl::error PAD 173 "No orientation information available for $name."
      }
    }
    set orient [dict get $cell_inst cell orient]

    if {[dict exists $cell_inst cell]} {
      dict set cell_inst cell [verify_placement [dict get $cell_inst cell] $cell_name cell]
    }

    # debug $cell_inst
    # debug "end"
    return $cell_inst
  }

  proc verify_placement {place cell_name {element "cell"}} {
    variable bondpad_width
    variable bondpad_height

    # debug "$element: $cell_name, place $place"
    if {[dict exists $place origin]} {
      dict set place scaled_origin [list \
        x [ord::microns_to_dbu [dict get $place origin x]] \
        y [ord::microns_to_dbu [dict get $place origin y]] \
      ]
      if {$element == "bondpad"} {
        set width $bondpad_width
        set height $bondpad_height
      } else {
        set cell [get_cell_master $cell_name]
        set width [$cell getWidth]
        set height [$cell getHeight]
      }
      set orient [dict get $place orient]

      set origin [dict get $place scaled_origin]
      dict set place scaled_center [get_center $origin $width $height $orient]
    }
    if {[dict exists $place center]} {
      dict set place scaled_center [list \
        x [ord::microns_to_dbu [dict get $place center x]] \
        y [ord::microns_to_dbu [dict get $place center y]] \
      ]
      if {$element == "bondpad"} {
        set width $bondpad_width
        set height $bondpad_height
      } else {
        set cell [get_cell_master $cell_name]
        set width [$cell getWidth]
        set height [$cell getHeight]
      }
      set orient [dict get $place orient]

      set center [dict get $place scaled_center]
      dict set place scaled_origin [get_origin $center $width $height $orient]
    }

    return $place
  }

  proc valid_edge_list {first_side} {
    set valid_keys {side {bottom right top left} corner {ll lr ur ul}}

    foreach edge_type {side corner} {
      if {[lsearch [dict get $valid_keys $edge_type] $first_side] > -1} {
        return [list $edge_type [dict get $valid_keys $edge_type]]
      }
    }
    error "incorrect specification $first_side, should be one of bottom, right, top, left, ll, lr, ur or ul"
  }

  proc check_side_specification {edges} {
    set first_side [lindex $edges 0]
    lassign [valid_edge_list $first_side] edge_type edge_list

    foreach edge $edges {
      if {[lsearch $edge_list $edge] == -1} {
        error "keyword $edge should be one of the following [join $edge_list {, }]"
      }
    }
  }

  proc check_cell_name_per_side {cell_name_by_side} {
    # debug $cell_name_by_side
    if {[catch {check_side_specification [dict keys $cell_name_by_side]} msg]} {
      utl::error PAD 174 "Unexpected keyword in cell name specification, $msg."
    }
    foreach side [dict keys $cell_name_by_side] {
      get_cell_master [dict get $cell_name_by_side $side]
    }

    return $cell_name_by_side
  }

  proc check_orient_per_side {orient_by_side} {
    if {[catch {check_side_specification [dict keys $orient_by_side]} msg]} {
      utl::error PAD 175 "Unexpected keyword in orient specification, $orient_by_side."
    }
    foreach side [dict keys $orient_by_side] {
      check_orient [dict get $orient_by_side $side]
    }

    return $orient_by_side
  }

  proc check_boolean {value} {
    if {$value == 1} {
      return 1
    }
    return 0
  }

  proc check_pad_pin_name {cell_name pin_name} {
    variable db

    if {[set master [$db findMaster $cell_name]] == "NULL"} {
      utl::error PAD 176 "Cannot find $cell_name in the database."
    }

    if {[$master findMTerm $pin_name] == "NULL"} {
      utl::error PAD 177 "Pin $pin_name does not exist on cell $cell_name."
    }
  }

  proc add_libcell {args} {
    variable library
    variable block
    variable type_index
    variable db

    set db [::ord::get_db]
    set tech [$db getTech]

    set cell_ref_name {}

    if {[llength $args] % 2 == 1} {
        utl::error PAD 178 "Incorrect number of arguments for add_pad, expected an even number, received [llength $args]."
    }

    set cell_ref {}
    # Determine the cell_ref name
    if {[dict exists $args -name]} {
      set cell_ref_name [dict get $args -name]
    } else {
      utl::error PAD 179 "Must specify -name option for add_libcell."
    }
    dict set cell_ref name $cell_ref_name

    set process_args $args
    while {[llength $process_args] > 0} {
      set arg [lindex $process_args 0]
      set value [lindex $process_args 1]

      switch $arg {
        -name          {dict set cell_ref name $value}
        -type          {dict set cell_ref type $value}
        -cell_name     {dict set cell_ref cell_name $value}
        -orient        {dict set cell_ref orient [check_orient_per_side $value]}
        -pad_pin_name  {dict set cell_ref pad_pin_name $value}
        -physical_only {dict set cell_ref physical_only [check_boolean $value]}
        -break_signals {dict set cell_ref breaks $value}
        default {utl::error PAD 201 "Unrecognized argument $arg, should be one of -name, -type, -cell_name, -orient, -pad_pin_name, -break_signals, -physical_only."}
      }

      set process_args [lrange $process_args 2 end]
    }

    if {[dict exists $args -signal]} {
      if {[set padcell_duplicate [find_padcell_with_signal_name [dict get $padcell signal_name]]] != {}} {
        utl::error PAD 202 "Padcell $padcell_duplicate already defined to use [dict get $padcell signal_name]."
      }
    }
    set cell_ref [verify_cell_ref $cell_ref]
    dict set library cells [dict get $cell_ref name] $cell_ref
  }

  proc verify_cell_ref {cell_ref} {
    variable db
    variable library

    # Verify name
    if {![dict exists $cell_ref name]} {
      utl::error PAD 180 "Library cell reference missing name attribute."
    }
    set cell_ref_name [dict get $cell_ref name]

    # Verify type
    if {![dict exists $cell_ref type]} {
      # Try to determine type
      if {[dict exists $library types]} {
        dict for {type cell_refs} [dict get $library types] {
          if {[lsearch $cell_refs $cell_ref_name] > -1} {
            dict set cell_ref type $type
            break
          }
        }
      }
      if {![dict exists $cell_ref type]} {
        utl::error PAD 181 "Library cell reference $cell_ref_name missing type attribute."
      }
    } else {
      set type [dict get $cell_ref type]
      if {[dict exists $library types $type]} {
        if {[dict get $library types $type] != $cell_ref_name} {
          if {$type == "fill"} {
            set types [dict get $library types $type]
            lappend types $cell_ref_name
            dict set library types $type $types
          } else {
            utl::error PAD 182 "Type of $cell_ref_name ($type) clashes with existing setting for type ([dict get $library types $type])."
          }
        }
      } else {
        dict set library types $type $cell_ref_name
      }
    }
    set type [dict get $cell_ref type]

    # Verify cell_name
    if {[dict exists $cell_ref cell_name]} {
      if {[llength [dict get $cell_ref cell_name]] > 1} {
        dict set cell_ref cell_name [check_cell_name_per_side [dict get $cell_ref cell_name]]
      } else {
        set cell_name [dict get $cell_ref cell_name]
        get_cell_master $cell_name
        if {$type == "bump"} {
          dict set cell_ref cell_name [check_cell_name $cell_name]
        } else {
          if {$type == "corner"} {
            set sides "ll lr ur ul"
          } else {
            set sides "bottom right top left"
          }
          dict set cell_ref cell_name {}
          foreach side $sides {
            dict set cell_ref cell_name $side $cell_name
          }
        }
      }
    } else {
      if {[$db findMaster $cell_ref_name] != "NULL"} {
        if {$type == "corner"} {
          set sides "ll lr ur ul"
        } else {
          set sides "bottom right top left"
        }
        foreach side $sides {
          dict set cell_ref cell_name $side $cell_ref_name
        }
      } else {
        utl::error PAD 183 "No specification found for which cell names to use on each side for padcell $cell_ref_name."
      }
    }
    set cell_name [dict get $cell_ref cell_name]

    # Verify orientation
    if {[dict exists $cell_ref orient]} {
      if {$type != "bump"} {
        dict set cell_ref orient [check_orient_per_side [dict get $cell_ref orient]]
      } else {
        check_orient [dict get $cell_ref orient]
      }
    } else {
      if {$type != "bump"} {
        # utl::error PAD 184 "No specification found for the orientation of cells on each side."
      } else {
        dict set cell_ref orient "R0"
      }
    }

    # Verify physical_only
    if {[dict exists $cell_ref physical_only]} {
      dict set cell_ref physical_only [check_boolean [dict get $cell_ref physical_only]]
    } else {
      dict set cell_ref physical_only 0
    }
    set physical_only [dict get $cell_ref physical_only]

    # Verify pad_pin_name
    if {![dict exists $cell_ref pad_pin_name]} {
      if {[dict exists $library pad_pin_name]} {
        dict set cell_ref pad_pin_name [dict get $library pad_pin_name]
      } else {
        if {$physical_only != 1} {
          utl::error PAD 185 "No specification of the name of the external pin on cell_ref $cell_ref_name."
        }
      }
    }
    if {$physical_only != 1} {
      dict for {side name} $cell_name {
        check_pad_pin_name $name [dict get $cell_ref pad_pin_name]
      }
    }

    # Verify breaks
    if {[dict exists $cell_ref breaks]} {
      dict lappend library breakers $type
    }

    return $cell_ref
  }

  proc check_type {type} {
    if {[regexp {[Ww]irebond} $type]} {
      return "wirebond"
    }
    if {[regexp {[Ff]lipchip} $type]} {
      return "flipchip"
    }
    utl::error PAD 113 "Type specified must be flipchip or wirebond."
  }

  proc set_type {type} {
    variable footprint

    dict set footprint type [check_type $type]
    if {[is_footprint_wirebond]} {
      init_library_bondpad
    } elseif {[is_footprint_flipchip]} {
      init_rdl
    }
}

  proc define {attribute args} {
    variable footprint

    dict set footprint $attribute {*}$args
  }

  proc library {attribute args} {
    variable library

    dict set library $attribute $args
  }

  proc define_ring {args} {
  }

  proc define_bumps {args} {
  }

  namespace export add_libcell define_ring define_bumps
  namespace export set_type set_die_area set_core_area set_offsets set_pin_layer set_pad_inst_name set_pad_pin_name set_rdl_cover_file_name
  namespace export add_pad add_ground_nets add_power_nets
  namespace export set_footprint set_library

  namespace export get_die_area get_core_area
  namespace export init_footprint load_footprint load_library_file
  namespace export extract_footprint write_footprint write_signal_mapping
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


sta::define_cmd_args "place_cell" {-inst_name inst_name \
                                     [-cell library_cell] \
                                     -origin xy_origin \
                                     -orient (R0|R90|R180|R270|MX|MY|MXR90|MYR90) \
                                     [-status (PLACED|FIRM)]}
proc place_cell {args} {
  if {[ord::get_db_block] == "NULL"} {
    utl::error PAD 228 "Design must be loaded before calling place_cell."
  }

  set db [ord::get_db]
  set block [ord::get_db_block]

  sta::parse_key_args "place_cell" args \
    keys {-cell -origin -orient -inst_name -status}

  if {[info exists keys(-status)]} {
    set placement_status $keys(-status)
    if {[lsearch {PLACED FIRM} $placement_status] == -1} {
      utl::error PAD 188 "Invalid placement status $placement_status, must be one of either PLACED or FIRM."
    }
  } else {
    set placement_status "PLACED"
  }

  if {[info exists keys(-cell)]} {
    set cell_name $keys(-cell)
    if {[set cell_master [$db findMaster $cell_name]] == "NULL"} {
      utl::error PAD 189 "Cell $cell_name not loaded into design."
    }
  }

  if {[info exists keys(-inst_name)]} {
    set inst_name [lindex $keys(-inst_name) 0]
  } else {
    utl::err PAD 190 "-inst_name is a required argument to the place_cell command."
  }

  # Verify cell orientation
  set valid_orientation {R0 R90 R180 R270 MX MY MXR90 MYR90}
  if {[info exists keys(-orient)]} {
    set orient $keys(-orient)
    if {[lsearch $valid_orientation $orient] == -1} {
      utl::error PAD 191 "Invalid orientation $orient specified, must be one of [join $valid_orientation {, }]."
    }
  } else {
    utl::error PAD 192 "No orientation specified for $inst_name."
  }

  # Verify center/origin
  if {[info exists keys(-origin)]} {
    set origin $keys(-origin)
    if {[llength $origin] != 2} {
      utl::error PAD 193 "Origin is $origin, but must be a list of 2 numbers."
    }
    if {[catch {set x [ord::microns_to_dbu [lindex $origin 0]]} msg]} {
      utl::error PAD 194 "Invalid value specified for x value, [lindex $origin 0], $msg."
    }
    if {[catch {set y [ord::microns_to_dbu [lindex $origin 1]]} msg]} {
      utl::error PAD 195 "Invalid value specified for y value, [lindex $origin 1], $msg."
    }
  } else {
    utl::error PAD 196 "No origin specified for $inst_name."
  }

  if {[set inst [$block findInst $inst_name]] == "NULL"} {
    if {[info exists keys(-cell)]} {
      set inst [odb::dbInst_create $block $cell_master $inst_name]
    } else {
      utl::error PAD 197 "Instance $inst_name not in the design, -cell must be specified to create a new instance."
    }
  } else {
    if {[info exists keys(-cell)]} {
      if {[[$inst getMaster] getName] != $cell_name} {
        utl::error PAD 198 "Instance $inst_name expected to be $cell_name, but is actually [[$inst getMaster] getName]."
      }
    }
  }

  if {$inst == "NULL"} {
    utl::error PAD 199 "Cannot create instance $inst_name of $cell_name."
  }

  $inst setOrigin $x $y
  $inst setOrient $orient
  $inst setPlacementStatus $placement_status
}
