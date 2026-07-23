# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2026, The OpenROAD Authors

sta::define_cmd_args "web_server" { [-port port] [-dir dir] [-stop] }

proc web_server { args } {
  sta::parse_key_args "web_server" args \
    keys {-port -dir} flags {-stop}

  if { [info exists flags(-stop)] } {
    web::web_server_stop_cmd
    return
  }

  sta::check_argc_eq0 "web_server" $args

  set port 0
  if { [info exists keys(-port)] } {
    set port $keys(-port)
  }

  if { [info exists keys(-dir)] } {
    utl::warn WEB 37 "-dir is deprecated and ignored; assets are embedded in the binary."
  }

  web::web_server_cmd $port
  web::web_server_wait_cmd
}

sta::define_cmd_args "save_image" {[-web] \
                                   [-area {x0 y0 x1 y1}] \
                                   [-width width] \
                                   [-resolution microns_per_pixel] \
                                   [-display_option option] \
                                   path
}

proc save_image { args } {
  ord::parse_list_args "save_image" args list {-display_option}
  sta::parse_key_args "save_image" args \
    keys {-area -width -resolution} flags {-web}

  set use_web [info exists flags(-web)]

  set resolution 0
  if { [info exists keys(-resolution)] } {
    sta::check_positive_float "-resolution" $keys(-resolution)
    set db [ord::get_db]
    set resolution [expr $keys(-resolution) * [$db getDbuPerMicron]]
    if { $resolution < 1 } {
      set resolution 1.0
      set res_per_pixel [expr $resolution / [$db getDbuPerMicron]]
      utl::warn WEB 25 "Resolution too high for design, defaulting to ${res_per_pixel}um per pixel"
    }
  }

  set area "0 0 0 0"
  if { [info exists keys(-area)] } {
    set area $keys(-area)
    if { [llength $area] != 4 } {
      utl::error WEB 26 "Area must contain 4 elements."
    }
  }

  set width 0
  if { [info exists keys(-width)] } {
    if { $resolution != 0 } {
      utl::error WEB 27 "Cannot set -width if -resolution has already been specified."
    }
    sta::check_positive_int "-width" $keys(-width)
    set width $keys(-width)
    if { $width == 0 } {
      utl::error WEB 29 "Specified -width cannot be zero."
    }
  }

  sta::check_argc_eq1 "save_image" $args
  set path [lindex $args 0]

  if { $use_web } {
    # Convert area from microns to DBU for the web renderer.
    set web_area $area
    if {
      [lindex $area 0] != 0 || [lindex $area 1] != 0
      || [lindex $area 2] != 0 || [lindex $area 3] != 0
    } {
      set db [ord::get_db]
      set dbu [$db getDbuPerMicron]
      set web_area [list \
        [expr { int([lindex $area 0] * $dbu) }] \
        [expr { int([lindex $area 1] * $dbu) }] \
        [expr { int([lindex $area 2] * $dbu) }] \
        [expr { int([lindex $area 3] * $dbu) }]]
    }

    # Build visibility JSON from display options.
    set vis_json ""
    if { [llength $list(-display_option)] > 0 } {
      set pairs {}
      foreach opt $list(-display_option) {
        if { [llength $opt] != 2 } {
          utl::error WEB 28 "Display option must have 2 elements {control} {value}."
        }
        set key [lindex $opt 0]
        set val [lindex $opt 1]
        if { $val eq "true" || $val eq "1" } {
          set val 1
        } else {
          set val 0
        }
        lappend pairs "\"$key\":$val"
      }
      set vis_json "\{[join $pairs ,]\}"
    }

    web::save_image_cmd $path \
      [lindex $web_area 0] [lindex $web_area 1] \
      [lindex $web_area 2] [lindex $web_area 3] \
      $width $resolution $vis_json
  } else {
    # Dispatch to the GUI renderer.
    set options [gui::DisplayControlMap]
    foreach opt $list(-display_option) {
      if { [llength $opt] != 2 } {
        utl::error GUI 19 "Display option must have 2 elements {control name} {value}."
      }
      $options set [lindex $opt 0] [lindex $opt 1]
    }

    gui::save_image $path {*}$area $width $resolution $options

    rename $options ""
  }
}

sta::define_cmd_args "web_save_report" {[-setup_paths count] \
                                        [-hold_paths count] \
                                        path
}

proc web_save_report { args } {
  sta::parse_key_args "web_save_report" args \
    keys {-setup_paths -hold_paths} flags {}

  set max_setup 100
  if { [info exists keys(-setup_paths)] } {
    sta::check_positive_int "-setup_paths" $keys(-setup_paths)
    set max_setup $keys(-setup_paths)
  }

  set max_hold 100
  if { [info exists keys(-hold_paths)] } {
    sta::check_positive_int "-hold_paths" $keys(-hold_paths)
    set max_hold $keys(-hold_paths)
  }

  sta::check_argc_eq1 "web_save_report" $args
  set path [lindex $args 0]

  web::save_report_cmd $path $max_setup $max_hold
}

sta::define_cmd_args "save_animated_gif" {(-start|-add|-end) \
                                         [-area {x0 y0 x1 y1}] \
                                         [-width width] \
                                         [-resolution microns_per_pixel] \
                                         [-delay delay] \
                                         [-key key] \
                                         [-display_option option] \
                                         [path]
}

proc save_animated_gif { args } {
  ord::parse_list_args "save_animated_gif" args list {-display_option}
  sta::parse_key_args "save_animated_gif" args \
    keys {-area -width -resolution -delay -key} \
    flags {-start -add -end}

  # -start opens a new GIF and returns its key.
  if { [info exists flags(-start)] } {
    sta::check_argc_eq1 "save_animated_gif" $args
    return [web::gif_start_cmd [lindex $args 0]]
  }

  sta::check_argc_eq0 "save_animated_gif" $args

  set key -1
  if { [info exists keys(-key)] } {
    set key $keys(-key)
  }

  # -end finalizes the GIF file.
  if { [info exists flags(-end)] } {
    web::gif_end_cmd $key
    return
  }

  if { ![info exists flags(-add)] } {
    utl::error WEB 60 "One of -start, -add or -end is required."
  }

  # -add captures one frame.  Convert -resolution (microns/pixel) to
  # dbu-per-pixel, mirroring save_image.
  set resolution 0
  if { [info exists keys(-resolution)] } {
    sta::check_positive_float "-resolution" $keys(-resolution)
    set db [ord::get_db]
    set resolution [expr $keys(-resolution) * [$db getDbuPerMicron]]
    if { $resolution < 1 } {
      set resolution 1.0
    }
  }

  set area "0 0 0 0"
  if { [info exists keys(-area)] } {
    set area $keys(-area)
    if { [llength $area] != 4 } {
      utl::error WEB 61 "Area must contain 4 elements."
    }
  }

  set width 0
  if { [info exists keys(-width)] } {
    if { $resolution != 0 } {
      utl::error WEB 62 "Cannot set -width if -resolution has already been specified."
    }
    sta::check_positive_int "-width" $keys(-width)
    set width $keys(-width)
  }

  set delay 0
  if { [info exists keys(-delay)] } {
    sta::check_positive_int "-delay" $keys(-delay)
    set delay $keys(-delay)
  }

  # Convert area from microns to DBU for the web renderer.
  set web_area $area
  if {
    [lindex $area 0] != 0 || [lindex $area 1] != 0
    || [lindex $area 2] != 0 || [lindex $area 3] != 0
  } {
    set db [ord::get_db]
    set dbu [$db getDbuPerMicron]
    set web_area [list \
      [expr { int([lindex $area 0] * $dbu) }] \
      [expr { int([lindex $area 1] * $dbu) }] \
      [expr { int([lindex $area 2] * $dbu) }] \
      [expr { int([lindex $area 3] * $dbu) }]]
  }

  # Build visibility JSON from display options (same scheme as save_image).
  set vis_json ""
  if { [llength $list(-display_option)] > 0 } {
    set pairs {}
    foreach opt $list(-display_option) {
      if { [llength $opt] != 2 } {
        utl::error WEB 63 "Display option must have 2 elements {control} {value}."
      }
      set okey [lindex $opt 0]
      set oval [lindex $opt 1]
      if { $oval eq "true" || $oval eq "1" } {
        set oval 1
      } else {
        set oval 0
      }
      lappend pairs "\"$okey\":$oval"
    }
    set vis_json "\{[join $pairs ,]\}"
  }

  web::gif_add_cmd $key \
    [lindex $web_area 0] [lindex $web_area 1] \
    [lindex $web_area 2] [lindex $web_area 3] \
    $width $resolution $delay $vis_json
}

sta::define_cmd_args "create_toolbar_button" {[-name name] \
                                              -text button_text \
                                              -script tcl_script \
                                              [-icon icon] \
                                              [-tooltip tooltip] \
                                              [-toggle] \
                                              [-script_off tcl_script_off] \
                                              [-echo]
}

proc create_toolbar_button { args } {
  sta::parse_key_args "create_toolbar_button" args \
    keys {-name -text -script -icon -tooltip -script_off} \
    flags {-toggle -echo}

  if { ![info exists keys(-text)] } {
    utl::error WEB 47 "-text is required."
  }
  if { ![info exists keys(-script)] } {
    utl::error WEB 48 "-script is required."
  }

  set name ""
  if { [info exists keys(-name)] } {
    set name $keys(-name)
  }
  set icon ""
  if { [info exists keys(-icon)] } {
    set icon $keys(-icon)
  }
  set tooltip ""
  if { [info exists keys(-tooltip)] } {
    set tooltip $keys(-tooltip)
  }
  set script_off ""
  if { [info exists keys(-script_off)] } {
    set script_off $keys(-script_off)
  }
  set toggle [info exists flags(-toggle)]
  set echo [info exists flags(-echo)]

  return [web::create_toolbar_button_cmd $name $keys(-text) $keys(-script) \
    $icon $tooltip $toggle $script_off $echo]
}

sta::define_cmd_args "create_menu_item" {[-name name] \
                                         [-path menu_path] \
                                         -text item_text \
                                         -script tcl_script \
                                         [-shortcut shortcut] \
                                         [-echo]
}

proc create_menu_item { args } {
  sta::parse_key_args "create_menu_item" args \
    keys {-name -path -text -script -shortcut} \
    flags {-echo}

  if { ![info exists keys(-text)] } {
    utl::error WEB 49 "-text is required."
  }
  if { ![info exists keys(-script)] } {
    utl::error WEB 50 "-script is required."
  }

  set name ""
  if { [info exists keys(-name)] } {
    set name $keys(-name)
  }
  set path ""
  if { [info exists keys(-path)] } {
    set path $keys(-path)
  }
  set shortcut ""
  if { [info exists keys(-shortcut)] } {
    set shortcut $keys(-shortcut)
  }
  set echo [info exists flags(-echo)]

  return [web::create_menu_item_cmd $name $path $keys(-text) $keys(-script) \
    $shortcut $echo]
}

sta::define_cmd_args "remove_toolbar_button" { name }

proc remove_toolbar_button { args } {
  sta::check_argc_eq1 "remove_toolbar_button" $args
  web::remove_toolbar_button_cmd [lindex $args 0]
}

sta::define_cmd_args "remove_menu_item" { name }

proc remove_menu_item { args } {
  sta::check_argc_eq1 "remove_menu_item" $args
  web::remove_menu_item_cmd [lindex $args 0]
}
