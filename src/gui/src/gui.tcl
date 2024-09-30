#############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2020, The Regents of the University of California
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
#############################################################################

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
    set tech [ord::get_db_tech]
    if { $tech == "NULL" } {
      utl::error GUI 17 "No technology loaded."
    }
    set resolution [expr $keys(-resolution) * [$tech getLefUnits]]
    if { $resolution < 1 } {
      set resolution 1.0
      set res_per_pixel [expr $resolution / [$tech getLefUnits]]
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

sta::define_cmd_args "save_clocktree_image" {
  [-width width] \
  [-height height] \
  [-corner corner] \
  -clock clock \
  path
}

proc save_clocktree_image { args } {
  sta::parse_key_args "save_clocktree_image" args \
    keys {-clock -width -height -corner} flags {}

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
  set corner ""
  if { [info exists keys(-corner)] } {
    set corner $keys(-corner)
  }

  if { [info exists keys(-clock)] } {
    set clock $keys(-clock)
  } else {
    utl:error GUI 88 "-clock is required"
  }

  gui::save_clocktree_image $path $clock $corner $width $height
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
