# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2020-2025, The OpenROAD Authors

sta::define_cmd_args "create_toolbar_button" {[-name name] \
                                              -text button_text \
                                              -script tcl_script \
                                              [-echo]
}

proc create_toolbar_button { args } {
  sta::parse_key_args "create_toolbar_button" args \
    keys {-name -text -script} flags {-echo}

  if { [info exists keys(-text)] } {
    set button_text $keys(-text)
  } else {
    utl::error GUI 20 "The -text argument must be specified."
  }
  if { [info exists keys(-script)] } {
    set tcl_script $keys(-script)
  } else {
    utl::error GUI 21 "The -script argument must be specified."
  }
  set name ""
  if { [info exists keys(-name)] } {
    set name $keys(-name)
  }
  set echo [info exists flags(-echo)]

  return [gui::create_toolbar_button $name $button_text $tcl_script $echo]
}

sta::define_cmd_args "create_menu_item" {[-name name] \
                                         -text item_text \
                                         -script tcl_script \
                                         [-path menu_path] \
                                         [-shortcut key_shortcut] \
                                         [-echo]
}

proc create_menu_item { args } {
  sta::parse_key_args "create_menu_item" args \
    keys {-name -text -script -shortcut -path} flags {-echo}

  if { [info exists keys(-text)] } {
    set action_text $keys(-text)
  } else {
    utl::error GUI 26 "The -text argument must be specified."
  }
  if { [info exists keys(-script)] } {
    set tcl_script $keys(-script)
  } else {
    utl::error GUI 27 "The -script argument must be specified."
  }
  set name ""
  if { [info exists keys(-name)] } {
    set name $keys(-name)
  }
  set shortcut ""
  if { [info exists keys(-shortcut)] } {
    set shortcut $keys(-shortcut)
  }
  set path ""
  if { [info exists keys(-path)] } {
    set path $keys(-path)
  }
  set echo [info exists flags(-echo)]

  return [gui::create_menu_item $name $path $action_text $tcl_script $shortcut $echo]
}

sta::define_cmd_args "save_image" {[-area {x0 y0 x1 y1}] \
                                   [-width width] \
                                   [-resolution microns_per_pixel] \
                                   [-display_option option] \
                                   path
} ;# checker off

proc save_image { args } {
  ord::parse_list_args "save_image" args list {-display_option}
  sta::parse_key_args "save_image" args \
    keys {-area -width -resolution} flags {} ;# checker off

  set options [gui::DisplayControlMap]
  foreach opt $list(-display_option) {
    if { [llength $opt] != 2 } {
      utl::error GUI 19 "Display option must have 2 elements {control name} {value}."
    }

    set key [lindex $opt 0]
    set val [lindex $opt 1]

    $options set $key $val
  }

  set resolution 0
  if { [info exists keys(-resolution)] } {
    sta::check_positive_float "-resolution" $keys(-resolution)
    set db [ord::get_db]
    set resolution [expr $keys(-resolution) * [$db getDbuPerMicron]]
    if { $resolution < 1 } {
      set resolution 1.0
      set res_per_pixel [expr $resolution / [$db getDbuPerMicron]]
      utl::warn GUI 31 "Resolution too high for design, defaulting to ${res_per_pixel}um per pixel"
    }
  }

  set area "0 0 0 0"
  if { [info exists keys(-area)] } {
    set area $keys(-area)
    if { [llength $area] != 4 } {
      utl::error GUI 18 "Area must contain 4 elements."
    }
  }

  set width 0
  if { [info exists keys(-width)] } {
    if { $resolution != 0 } {
      utl::error GUI 96 "Cannot set -width if -resolution has already been specified."
    }
    sta::check_positive_int "-width" $keys(-width)
    set width $keys(-width)
    if { $width == 0 } {
      utl::error GUI 98 "Specified -width cannot be zero."
    }
  }

  sta::check_argc_eq1 "save_image" $args
  set path [lindex $args 0]

  gui::save_image $path {*}$area $width $resolution $options

  # delete map
  rename $options ""
}

sta::define_cmd_args "save_animated_gif" {-start|-add|-end \
                                          [-area {x0 y0 x1 y1}] \
                                          [-width width] \
                                          [-resolution microns_per_pixel] \
                                          [-delay delay] \
                                          [-key key] \
                                          [path]
}

proc save_animated_gif { args } {
  sta::parse_key_args "save_animated_gif" args \
    keys {-area -width -resolution -delay -key} flags {-start -end -add}

  set resolution 0
  if { [info exists keys(-resolution)] } {
    sta::check_positive_float "-resolution" $keys(-resolution)
    set db [ord::get_db]
    set resolution [expr $keys(-resolution) * [$db getDbuPerMicron]]
    if { $resolution < 1 } {
      set resolution 1.0
      set res_per_pixel [expr $resolution / [$db getDbuPerMicron]]
      utl::warn GUI 55 "Resolution too high for design, defaulting to ${res_per_pixel}um per pixel"
    }
  }

  set area "0 0 0 0"
  if { [info exists keys(-area)] } {
    set area $keys(-area)
    if { [llength $area] != 4 } {
      utl::error GUI 48 "Area must contain 4 elements."
    }
  }

  set width 0
  if { [info exists keys(-width)] } {
    if { $resolution != 0 } {
      utl::error GUI 99 "Cannot set -width if -resolution has already been specified."
    }
    sta::check_positive_int "-width" $keys(-width)
    set width $keys(-width)
    if { $width == 0 } {
      utl::error GUI 105 "Specified -width cannot be zero."
    }
  }

  set delay 0
  if { [info exists keys(-delay)] } {
    set delay $keys(-delay)
  }

  set key -1
  if { [info exists keys(-key)] } {
    set key $keys(-key)
  }

  if { [info exists flags(-start)] } {
    sta::check_argc_eq1 "save_animated_gif" $args
    set path [lindex $args 0]

    return [gui::gif_start $path]
  } elseif { [info exists flags(-add)] } {
    sta::check_argc_eq0 "save_animated_gif" $args

    gui::gif_add $key {*}$area $width $resolution $delay
  } elseif { [info exists flags(-end)] } {
    sta::check_argc_eq0 "save_animated_gif" $args

    gui::gif_end $key
  } else {
    utl::error GUI 106 "-start, -end, or -add is required"
  }
}

sta::define_cmd_args "save_clocktree_image" {
  [-width width] \
  [-height height] \
  [-scene scene] \
  -clock clock \
  path
}

proc save_clocktree_image { args } {
  sta::parse_key_args "save_clocktree_image" args \
    keys {-clock -width -height -scene -corner} flags {}

  sta::check_argc_eq1 "save_clocktree_image" $args
  set path [lindex $args 0]

  set width 0
  if { [info exists keys(-width)] } {
    set width $keys(-width)
  }
  set height 0
  if { [info exists keys(-height)] } {
    set height $keys(-height)
  }
  set scene [sta::parse_scene keys]

  if { [info exists keys(-clock)] } {
    set clock $keys(-clock)
  } else {
    utl::error GUI 88 "-clock is required"
  }

  gui::save_clocktree_image $path $clock $scene $width $height
}

sta::define_cmd_args "save_histogram_image" {
  [-width width] \
  [-height height] \
  [-mode mode] \
  path
}

proc save_histogram_image { args } {
  sta::parse_key_args "save_histogram_image" args \
    keys {-width -height -mode} flags {}

  sta::check_argc_eq1 "save_histogram_image" $args
  set path [lindex $args 0]

  set width 0
  if { [info exists keys(-width)] } {
    set width $keys(-width)
  }
  set height 0
  if { [info exists keys(-height)] } {
    set height $keys(-height)
  }
  set mode "setup"
  if { [info exists keys(-mode)] } {
    set mode $keys(-mode)
  }

  gui::save_histogram_image $path $mode $width $height
}

sta::define_cmd_args "select" {-type object_type \
                               [-name name_regex] \
                               [-case_insensitive] \
                               [-highlight group] \
                               [-filter attribute_and_value]
}

proc select { args } {
  sta::parse_key_args "select" args \
    keys {-type -name -highlight -filter} flags {-case_insensitive}
  sta::check_argc_eq0 "select" $args

  set type ""
  if { [info exists keys(-type)] } {
    set type $keys(-type)
  } else {
    utl::error GUI 38 "Must specify -type."
  }

  set highlight -1
  if { [info exists keys(-highlight)] } {
    set highlight $keys(-highlight)
  }

  set name ""
  if { [info exists keys(-name)] } {
    set name $keys(-name)
  }

  set attribute ""
  set value ""
  if { [info exists keys(-filter)] } {
    set filter $keys(-filter)
    set filter [split $filter "="]
    if { [llength $filter] != 2 } {
      utl::error GUI 56 "Invalid syntax for -filter. Use -filter attribute=value."
    }
    set attribute [lindex $filter 0]
    set value [lindex $filter 1]
  }

  set case_sense 1
  if { [info exists flags(-case_insensitive)] } {
    if { $name == "" } {
      utl::warn GUI 39 "Cannot use case insensitivity without a name."
    }
    set case_sense 0
  }

  return [gui::select $type $name $attribute $value $case_sense $highlight]
}

sta::define_cmd_args "display_timing_cone" {pin \
                                            [-fanin] \
                                            [-fanout] \
                                            [-off]
}

proc display_timing_cone { args } {
  sta::parse_key_args "display_timing_cone" args \
    keys {} flags {-fanin -fanout -off}
  if { [info exists flags(-off)] } {
    sta::check_argc_eq0 "timing_cone" $args

    gui::timing_cone NULL 0 0
    return
  }

  sta::check_argc_eq1 "select" $args

  set fanin [info exists flags(-fanin)]
  set fanout [info exists flags(-fanout)]

  # clear old one
  gui::timing_cone NULL 0 0

  set block [ord::get_db_block]
  if { $block == "NULL" } {
    utl::error GUI 67 "Design not loaded."
  }

  sta::parse_port_pin_net_arg $args pins nets

  foreach net $nets {
    lappend pins [sta::net_load_pins $net]
  }
  if { [llength $pins] == 0 } {
    utl::error GUI 68 "Pin not found."
  }
  if { [llength $pins] != 1 } {
    utl::error GUI 69 "Multiple pin timing cones are not supported."
  }

  set term [sta::sta_to_db_pin $pins]
  if { $term == "NULL" } {
    set term [sta::sta_to_db_port $pins]
  }

  # select new one
  gui::timing_cone $term $fanin $fanout
}

sta::define_cmd_args "focus_net" {net \
                                  [-remove] \
                                  [-clear]
}

proc focus_net { args } {
  sta::parse_key_args "focus_net" args \
    keys {} flags {-remove -clear}
  if { [info exists flags(-clear)] } {
    sta::check_argc_eq0 "focus_net" $args

    gui::clear_focus_nets
    return
  }

  sta::check_argc_eq1 "focus_net" $args

  set block [ord::get_db_block]
  if { $block == "NULL" } {
    utl::error GUI 70 "Design not loaded."
  }

  set net_name [lindex $args 0]
  set net [$block findNet $net_name]

  if { $net == "NULL" } {
    utl::error GUI 71 "Unable to find net \"$net_name\"."
  }

  if { [info exists flags(-remove)] } {
    gui::remove_focus_net $net
  } else {
    gui::focus_net $net
  }
}

sta::define_cmd_args "add_label" {-position {x y}
                                  [-anchor anchor]
                                  [-color color]
                                  [-size size]
                                  [-name name]
                                  text
}

proc add_label { args } {
  sta::parse_key_args "add_label" args \
    keys {-position -anchor -color -size -name} flags {}

  sta::check_argc_eq1 "add_label" $args

  if { ![info exists keys(-position)] } {
    utl::error GUI 46 "-position is required"
  }
  set pos $keys(-position)
  if { [llength $pos] != 2 } {
    utl::error GUI 47 "-position must contain x and y"
  }

  set anchor ""
  if { [info exists keys(-anchor)] } {
    set anchor $keys(-anchor)
  }

  set color ""
  if { [info exists keys(-color)] } {
    set color $keys(-color)
  }

  set size 0
  if { [info exists keys(-size)] } {
    set size $keys(-size)
  }

  set name ""
  if { [info exists keys(-name)] } {
    set name $keys(-name)
  }

  return [gui::add_label \
    [lindex $pos 0] \
    [lindex $pos 1] \
    [lindex $args 0] \
    $anchor \
    $color \
    $size \
    $name]
}

namespace eval gui {
proc show_worst_path { args } {
  sta::parse_key_args "show_worst_path" args \
    keys {} flags {-setup -hold}

  sta::check_argc_eq0 "show_worst_path" $args

  set setup 1
  if { [info exists flags(-hold)] } {
    set setup 0
  }

  gui::show_worst_path_internal $setup
}

sta::define_cmd_args "clear_timing_path" {}

proc clear_timing_path { args } {
  sta::parse_key_args "clear_timing_path" args \
    keys {} flags {}

  sta::check_argc_eq0 "clear_timing_path" $args

  gui::clear_timing_path_internal
}
} ;# namespace gui
