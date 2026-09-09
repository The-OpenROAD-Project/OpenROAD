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
        # Emit real JSON booleans: TileVisibility::parseFromJson reads every
        # field with value_to<bool>, which rejects 1/0 — and the caller catches
        # that by discarding the WHOLE visibility object (WEB-0042), so one
        # integer used to turn every -display_option into a no-op.
        if { $val eq "true" || $val eq "1" } {
          set val "true"
        } else {
          set val "false"
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

# --- Viewer dispatch ---
#
# The Qt GUI defines add_label, create_toolbar_button, create_menu_item and
# save_animated_gif as global procs as well, and web.tcl is evaluated after
# gui.tcl, so whichever definition lands here is the only one a user can
# reach.  So rather than take the name over, each proc below keeps one Tcl
# interface and dispatches on the viewer that is actually up: the Qt main
# window when it is running, the web viewer otherwise.  That is the precedence
# gui::Gui itself applies between its main window and a HeadlessViewer, and it
# is why a Qt build launched without -gui lands on the web viewer rather than
# on the GUI command's "not usable in non-GUI mode".
#
# gui::has_ui is the predicate: gui.i exports it in a Qt build and stub.cpp
# defines it in a build without Qt, so it always answers.

namespace eval web {
# Options the web viewer adds that the Qt path cannot honour: its toolbar
# buttons are text-only and stateless, and its save_animated_gif takes no
# display options.  A warning rather than an error, so one script can drive
# either viewer.
proc warn_qt_unsupported { cmd unsupported } {
  if { [llength $unsupported] == 0 } {
    return
  }
  utl::warn WEB 76 "$cmd: the Qt GUI does not support [join $unsupported {, }]; ignored."
}
}

sta::define_cmd_args "add_label" {-position {x y} \
                                  [-anchor anchor] \
                                  [-color color] \
                                  [-size size] \
                                  [-name name] \
                                  text
}

proc add_label { args } {
  sta::parse_key_args "add_label" args \
    keys {-position -anchor -color -size -name} flags {}

  if { ![info exists keys(-position)] } {
    utl::error WEB 55 "-position is required."
  }
  set pos $keys(-position)
  if { [llength $pos] != 2 } {
    utl::error WEB 56 "-position must have 2 elements {x y}."
  }
  sta::check_argc_eq1 "add_label" $args
  set text [lindex $args 0]

  set anchor ""
  if { [info exists keys(-anchor)] } { set anchor $keys(-anchor) }
  set color ""
  if { [info exists keys(-color)] } { set color $keys(-color) }
  set size 0
  if { [info exists keys(-size)] } { set size $keys(-size) }
  set name ""
  if { [info exists keys(-name)] } { set name $keys(-name) }

  if { [gui::has_ui] } {
    # gui::add_label takes -position's microns and converts them itself.
    return [gui::add_label [lindex $pos 0] [lindex $pos 1] $text \
      $anchor $color $size $name]
  }

  # The web renderer takes DBU.
  set db [ord::get_db]
  set dbu [$db getDbuPerMicron]
  set x [expr { int([lindex $pos 0] * $dbu) }]
  set y [expr { int([lindex $pos 1] * $dbu) }]
  return [web::add_label_cmd $x $y $text $anchor $color $size $name]
}

sta::define_cmd_args "delete_label" { name }

proc delete_label { args } {
  sta::check_argc_eq1 "delete_label" $args
  if { [gui::has_ui] } {
    gui::delete_label [lindex $args 0]
    return
  }
  web::delete_label_cmd [lindex $args 0]
}

sta::define_cmd_args "clear_labels" {}

proc clear_labels { args } {
  sta::check_argc_eq0 "clear_labels" $args
  if { [gui::has_ui] } {
    gui::clear_labels
    return
  }
  web::clear_labels_cmd
}

sta::define_cmd_args "save_display_controls" { filename }

proc save_display_controls { args } {
  sta::parse_key_args "save_display_controls" args keys {} flags {}
  sta::check_argc_eq1 "save_display_controls" $args
  set path [lindex $args 0]

  web::save_display_controls_cmd $path
}

sta::define_cmd_args "restore_display_controls" { filename }

proc restore_display_controls { args } {
  sta::parse_key_args "restore_display_controls" args keys {} flags {}
  sta::check_argc_eq1 "restore_display_controls" $args
  set path [lindex $args 0]

  web::restore_display_controls_cmd $path
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
    if { [gui::has_ui] } {
      return [gui::gif_start [lindex $args 0]]
    }
    return [web::gif_start_cmd [lindex $args 0]]
  }

  sta::check_argc_eq0 "save_animated_gif" $args

  set key -1
  if { [info exists keys(-key)] } {
    set key $keys(-key)
  }

  # -end finalizes the GIF file.
  if { [info exists flags(-end)] } {
    if { [gui::has_ui] } {
      gui::gif_end $key
      return
    }
    web::gif_end_cmd $key
    return
  }

  if { ![info exists flags(-add)] } {
    utl::error WEB 63 "One of -start, -add or -end is required."
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
      utl::error WEB 64 "Area must contain 4 elements."
    }
  }

  set width 0
  if { [info exists keys(-width)] } {
    if { $resolution != 0 } {
      utl::error WEB 65 "Cannot set -width if -resolution has already been specified."
    }
    sta::check_positive_int "-width" $keys(-width)
    set width $keys(-width)
  }

  set delay 0
  if { [info exists keys(-delay)] } {
    sta::check_positive_int "-delay" $keys(-delay)
    set delay $keys(-delay)
  }

  if { [gui::has_ui] } {
    set unsupported {}
    if { [llength $list(-display_option)] > 0 } {
      lappend unsupported "-display_option"
    }
    web::warn_qt_unsupported "save_animated_gif" $unsupported
    # gui::gif_add takes the area in microns.
    gui::gif_add $key {*}$area $width $resolution $delay
    return
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
        utl::error WEB 66 "Display option must have 2 elements {control} {value}."
      }
      set okey [lindex $opt 0]
      set oval [lindex $opt 1]
      # Real JSON booleans, for the reason save_image spells out above: an
      # integer here makes TileVisibility::parseFromJson discard the whole
      # visibility object (WEB-0042), so every -display_option is a no-op.
      if { $oval eq "true" || $oval eq "1" } {
        set oval "true"
      } else {
        set oval "false"
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
    utl::error WEB 59 "-text is required."
  }
  if { ![info exists keys(-script)] } {
    utl::error WEB 60 "-script is required."
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

  if { [gui::has_ui] } {
    set unsupported {}
    foreach {opt val} [list -icon $icon -tooltip $tooltip \
      -script_off $script_off] {
      if { $val ne "" } {
        lappend unsupported $opt
      }
    }
    if { $toggle } {
      lappend unsupported "-toggle"
    }
    web::warn_qt_unsupported "create_toolbar_button" $unsupported
    return [gui::create_toolbar_button $name $keys(-text) $keys(-script) $echo]
  }

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
    utl::error WEB 61 "-text is required."
  }
  if { ![info exists keys(-script)] } {
    utl::error WEB 62 "-script is required."
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

  if { [gui::has_ui] } {
    return [gui::create_menu_item $name $path $keys(-text) $keys(-script) \
      $shortcut $echo]
  }

  return [web::create_menu_item_cmd $name $path $keys(-text) $keys(-script) \
    $shortcut $echo]
}

sta::define_cmd_args "remove_toolbar_button" { name }

proc remove_toolbar_button { args } {
  sta::check_argc_eq1 "remove_toolbar_button" $args
  if { [gui::has_ui] } {
    gui::remove_toolbar_button [lindex $args 0]
    return
  }
  web::remove_toolbar_button_cmd [lindex $args 0]
}

sta::define_cmd_args "remove_menu_item" { name }

proc remove_menu_item { args } {
  sta::check_argc_eq1 "remove_menu_item" $args
  if { [gui::has_ui] } {
    gui::remove_menu_item [lindex $args 0]
    return
  }
  web::remove_menu_item_cmd [lindex $args 0]
}
