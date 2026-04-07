# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2026, The OpenROAD Authors

sta::define_cmd_args "web_server" { [-port port] -dir dir }

proc web_server { args } {
  sta::parse_key_args "web_server" args \
    keys {-port -dir} flags {}

  sta::check_argc_eq0 "web_server" $args

  set port 8080
  if { [info exists keys(-port)] } {
    set port $keys(-port)
  }

  if { ![info exists keys(-dir)] } {
    utl::error WEB 19 "-dir is required: pass the path to the web assets directory."
  }

  web::web_server_cmd $port $keys(-dir)
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
