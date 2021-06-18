###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, University of California, San Diego.
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
##   and#or other materials provided with the distribution.
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
##
###############################################################################


sta::define_cmd_args "define_pin_shape_pattern" {[-layer layer] \
                                                 [-x_step x_step] \
                                                 [-y_step y_step] \
                                                 [-region region] \
                                                 [-size size] \
                                                 [-pin_keepout dist]}

proc define_pin_shape_pattern { args } {
  sta::parse_key_args "define_pin_shape_pattern" args \
  keys {-layer -x_step -y_step -region -size -pin_keepout}

  if [info exists keys(-layer)] {
    set layer_name $keys(-layer)
    set layer_idx [ppl::parse_layer_name $layer_name]

    if { $layer_idx == 0 } {
      utl::error PPL 52 "Routing layer not found for name $layer_name."
    }
  } else {
    utl::error PPL 53 "-layer is required."
  }

  if { [info exists keys(-x_step)] && [info exists keys(-y_step)] } {
    set x_step [ord::microns_to_dbu $keys(-x_step)]
    set y_step [ord::microns_to_dbu $keys(-y_step)]
  } else {
    utl::error PPL 54 "-x_step and -y_step are required."
  }

  if [info exists keys(-region)] {
    set region $keys(-region)
    if [regexp -all {([0-9]+[.]*[0-9]*) ([0-9]+[.]*[0-9]*) ([0-9]+[.]*[0-9]*) ([0-9]+[.]*[0-9]*)} $region - llx lly urx ury] {
      set llx [ord::microns_to_dbu $llx]
      set lly [ord::microns_to_dbu $lly]
      set urx [ord::microns_to_dbu $urx]
      set ury [ord::microns_to_dbu $ury]
    } elseif {$region == "*"} {
      set dbBlock [ord::get_db_block]
      set die_area [$dbBlock getDieArea]
      set llx [$die_area xMin]
      set lly [$die_area yMin]
      set urx [$die_area xMax]
      set ury [$die_area yMax]
    } else {
      utl::error PPL 63 "-region is not a list of 4 values {llx lly urx ury}."
    }
  } else {
    utl::error PPL 55 "-region is required."
  }

  if [info exists keys(-size)] {
    set size $keys(-size)
    if { [llength $size] != 2 } {
      utl::error PPL 56 "-size is not a list of 2 values."
    }
    lassign $size width height
    set width [ord::microns_to_dbu $width]
    set height [ord::microns_to_dbu $height]
  } else {
    utl::error PPL 57 "-size is required."
  }

  if [info exists keys(-pin_keepout)] {
    sta::check_positive_float "pin_keepout" $keys(-pin_keepout)
    set keepout [ord::microns_to_dbu $keys(-pin_keepout)]
  } else {
    set max_dim $width
    if {$max_dim < $height} {
      set max_dim $height
    }
    set keepout [[[ord::get_db_tech] findLayer $keys(-layer)] getSpacing $max_dim]
  }

  ppl::create_pin_shape_pattern $layer_idx $x_step $y_step $llx $lly $urx $ury $width $height $keepout
}

sta::define_cmd_args "set_io_pin_constraint" {[-direction direction] \
                                              [-pin_names names] \
                                              [-region region]}

proc set_io_pin_constraint { args } {
  sta::parse_key_args "set_io_pin_constraint" args \
  keys {-direction -pin_names -region}

  if [info exists keys(-region)] {
    set region $keys(-region)
  }

  set dbTech [ord::get_db_tech]
  set dbBlock [ord::get_db_block]
  set lef_units [$dbTech getLefUnits]

  if [regexp -all {(top|bottom|left|right):(.+)} $region - edge interval] {
    set edge_ [ppl::parse_edge "-region" $edge]

    if [regexp -all {([0-9]+[.]*[0-9]*|[*]+)-([0-9]+[.]*[0-9]*|[*]+)} $interval - begin end] {
      if {$begin == {*}} {
        set begin [ppl::get_edge_extreme "-region" 1 $edge]
      }
      if {$end == {*}} {
        set end [ppl::get_edge_extreme "-region" 0 $edge]
      }

      set begin [expr { int($begin * $lef_units) }]
      set end [expr { int($end * $lef_units) }]
    } elseif {$interval == {*}} {
      set begin [ppl::get_edge_extreme "-region" 1 $edge]
      set end [ppl::get_edge_extreme "-region" 0 $edge]
    }

    if {[info exists keys(-direction)] && [info exists keys(-pin_names)]} {
      utl::error PPL 16 "Both -direction and -pin_names constraints not allowed."
    }

    if [info exists keys(-direction)] {
      set direction $keys(-direction)
      set dir [ppl::parse_direction "set_io_pin_constraint" $direction]
      utl::info PPL 49 "Restrict $direction pins to region [ord::dbu_to_microns $begin]u-[ord::dbu_to_microns $end]u, in the $edge edge."
      ppl::add_direction_constraint $dir $edge_ $begin $end
    }

    if [info exists keys(-pin_names)] {
      set names $keys(-pin_names)
      ppl::add_pins_to_constraint "set_io_pin_constraint" $names $edge_ $begin $end $edge
    }
  } elseif [regexp -all {(up):(.*)} $region - edge box] {
    if {$box == "*"} {
      set die_area [$dbBlock getDieArea]
      set llx [$die_area xMin]
      set lly [$die_area yMin]
      set urx [$die_area xMax]
      set ury [$die_area yMax]
    } elseif [regexp -all {([0-9]+[.]*[0-9]*) ([0-9]+[.]*[0-9]*) ([0-9]+[.]*[0-9]*) ([0-9]+[.]*[0-9]*)} $box - llx lly urx ury] {
      set llx [ord::microns_to_dbu $llx]
      set lly [ord::microns_to_dbu $lly]
      set urx [ord::microns_to_dbu $urx]
      set ury [ord::microns_to_dbu $ury]
    } else {
      utl::error PPL 59 "Box at top layer must have 4 values (llx lly urx ury)."
    }

    if [info exists keys(-pin_names)] {
      set names $keys(-pin_names)
      ppl::add_pins_to_top_layer "set_io_pin_constraint" $names $llx $lly $urx $ury
    }
  } else {
    utl::warn PPL 73 "Constraint with region $region has an invalid edge."
  }
}

proc clear_io_pin_constraints {} {
  ppl::clear_constraints
}

sta::define_cmd_args "place_pin" {[-pin_name pin_name]\
                                  [-layer layer]\
                                  [-location location]\
                                  [-pin_size pin_size]
}

proc place_pin { args } {
  sta::parse_key_args "place_pin" args \
  keys {-pin_name -layer -location -pin_size}

  if [info exists keys(-pin_name)] {
    set pin_name $keys(-pin_name)
  } else {
    utl::error PPL 64 "-pin_name is required."
  }

  if [info exists keys(-layer)] {
    set layer $keys(-layer)
  } else {
    utl::error PPL 65 "-layer is required."
  }

  if [info exists keys(-location)] {
    set location $keys(-location)
  } else {
    utl::error PPL 66 "-location is required."
  }

  if { [llength $location] != 2 } {
    utl::error PPL 68 "-location is not a list of 2 values."
  }
  lassign $location x y
  set x [ord::microns_to_dbu $x]
  set y [ord::microns_to_dbu $y]

  if [info exists keys(-pin_size)] {
    set pin_size $keys(-pin_size)
  } else {
    utl::error PPL 67 "-pin_size is required."
  }

  if { [llength $pin_size] != 2 } {
    utl::error PPL 69 "-pin_size is not a list of 2 values."
  }
  lassign $pin_size width height
  set width [ord::microns_to_dbu $width]
  set height [ord::microns_to_dbu $height]

  set pin [ppl::parse_pin_names "place_pin" $pin_name]
  if { [llength $pin] > 1 } {
    utl::error PPL 71 "place_pin command should receive only one pin name."
  }

  set layer_idx [ppl::parse_layer_name $layer]

  ppl::place_pin $pin $layer_idx $x $y $width $height
}

sta::define_cmd_args "place_pins" {[-hor_layers h_layers]\
                                  [-ver_layers v_layers]\
                                  [-random_seed seed]\
                       	          [-random]\
                                  [-corner_avoidance distance]\
                                  [-min_distance min_dist]\
                                  [-exclude region]\
                                  [-group_pins pin_list]
                                 }

proc place_pins { args } {
  set regions [ppl::parse_excludes_arg $args]
  set pin_groups [ppl::parse_group_pins_arg $args]
  sta::parse_key_args "place_pins" args \
  keys {-hor_layers -ver_layers -random_seed -corner_avoidance -min_distance -exclude -group_pins} \
  flags {-random}

  set dbTech [ord::get_db_tech]
  if { $dbTech == "NULL" } {
    utl::error PPL 31 "No technology found."
  }

  set dbBlock [ord::get_db_block]
  if { $dbBlock == "NULL" } {
    utl::error PPL 32 "No block found."
  }

  set db [ord::get_db]
  
  set blockages {}

  foreach inst [$dbBlock getInsts] {
    if { [$inst isBlock] } {
      if { ![$inst isPlaced] } {
        utl::warn PPL 15 "Macro [$inst getName] is not placed."
      } else {
        lappend blockages $inst
      }
    }
  }

  utl::report "Found [llength $blockages] macro blocks."

  set seed 42
  if [info exists keys(-random_seed)] {
    set seed $keys(-random_seed)
  }
  ppl::set_rand_seed $seed

  if [info exists keys(-hor_layers)] {
    set hor_layers $keys(-hor_layers)
  } else {
    utl::error PPL 17 "-hor_layers is required."
  }       
  
  if [info exists keys(-ver_layers)] {
    set ver_layers $keys(-ver_layers)
  } else {
    utl::error PPL 18 "-ver_layers is required."
  }

  # set default interval_length from boundaries as 1u
  set distance 1
  if [info exists keys(-corner_avoidance)] {
    set distance $keys(-corner_avoidance)
    ppl::set_corner_avoidance [ord::microns_to_dbu $distance]
  } else {
    utl::report "Using ${distance}u default distance from corners."
    ppl::set_corner_avoidance [ord::microns_to_dbu $distance]
  }

  set min_dist 2
  if [info exists keys(-min_distance)] {
    set min_dist $keys(-min_distance)
    ppl::set_min_distance [ord::microns_to_dbu $min_dist]
  } else {
    utl::report "Using $min_dist tracks default min distance between IO pins."
    # setting min distance as 0u leads to the default min distance
    ppl::set_min_distance 0
  }

  set bterms_cnt [llength [$dbBlock getBTerms]]

  if { $bterms_cnt == 0 } {
    utl::error PPL 19 "Design without pins"
  }


  set num_tracks_y 0
  foreach hor_layer_name $hor_layers {
    set hor_layer [ppl::parse_layer_name $hor_layer_name]
    if { ![ord::db_layer_has_hor_tracks $hor_layer] } {
      utl::error PPL 21 "Horizontal routing tracks not found for layer $hor_layer_name."
    }

    set h_tech_layer [$dbTech findRoutingLayer $hor_layer]
    if { [$h_tech_layer getDirection] != "HORIZONTAL" } {
      utl::warn PPL 45 "Layer $hor_layer_name preferred direction is not horizontal."
    }

    set hor_track_grid [$dbBlock findTrackGrid $h_tech_layer]
    
    set num_tracks_y [expr $num_tracks_y+[llength [$hor_track_grid getGridY]]]

    ppl::add_hor_layer $hor_layer 
  }

  set num_tracks_x 0
  foreach ver_layer_name $ver_layers {
    set ver_layer [ppl::parse_layer_name $ver_layer_name]
    if { ![ord::db_layer_has_ver_tracks $ver_layer] } {
      utl::error PPL 23 "Vertical routing tracks not found for layer $ver_layer_name."
    }

    set v_tech_layer [$dbTech findRoutingLayer $ver_layer]
    if { [$v_tech_layer getDirection] != "VERTICAL" } {
      utl::warn PPL 46 "Layer $ver_layer_name preferred direction is not vertical."
    }

    set ver_track_grid [$dbBlock findTrackGrid $v_tech_layer]

    set num_tracks_x [expr $num_tracks_x+[llength [$ver_track_grid getGridX]]]

    ppl::add_ver_layer $ver_layer
  }
  
  set num_slots [expr (2*$num_tracks_x + 2*$num_tracks_y)/$min_dist]

  if { ($bterms_cnt > $num_slots) } {
    utl::error PPL 24 "Number of pins $bterms_cnt exceeds max possible $num_slots."
  }
 
  if { $regions != {} } {
    set lef_units [$dbTech getLefUnits]
    
    foreach region $regions {
      if [regexp -all {(top|bottom|left|right):(.+)} $region - edge interval] {
        set edge_ [ppl::parse_edge "-exclude" $edge]

        if [regexp -all {([0-9]+[.]*[0-9]*|[*]+)-([0-9]+[.]*[0-9]*|[*]+)} $interval - begin end] {
          if {$begin == {*}} {
            set begin [ppl::get_edge_extreme "-exclude" 1 $edge]
          }
          if {$end == {*}} {
            set end [ppl::get_edge_extreme "-exclude" 0 $edge]
          }
          set begin [expr { int($begin * $lef_units) }]
          set end [expr { int($end * $lef_units) }]

          ppl::exclude_interval $edge_ $begin $end
        } elseif {$interval == {*}} {
          set begin [ppl::get_edge_extreme "-exclude" 1 $edge]
          set end [ppl::get_edge_extreme "-exclude" 0 $edge]

          ppl::exclude_interval $edge_ $begin $end
        } else {
          utl::error PPL 25 "-exclude: $interval is an invalid region"
        }
      } else {
        utl::error PPL 26 "-exclude: invalid syntax in $region. Use (top|bottom|left|right):interval."
      }
    }
  }

  if { $pin_groups != {} } {
    set group_idx 0
    foreach group $pin_groups {
      utl::info PPL 41 "Pin group $group_idx: \[$group\]"
      set pin_list {}
      foreach pin_name $group {
        set db_bterm [$dbBlock findBTerm $pin_name]
        if { $db_bterm != "NULL" } {
          lappend pin_list $db_bterm
        } else {
          utl::warn PPL 43 "Pin $pin_name not found in group $group_idx"
        }
      }
      ppl::add_pin_group $pin_list
      incr group_idx
    }
  }
  
  ppl::run_io_placement [info exists flags(-random)]
}

namespace eval ppl {

proc parse_edge { cmd edge } {
  if {$edge != "top" && $edge != "bottom" && \
      $edge != "left" && $edge != "right"} {
    utl::error PPL 27 "$cmd: $edge is an invalid edge. Use top, bottom, left or right."
  }
  return [ppl::get_edge $edge]
}

proc parse_direction { cmd direction } {
  if {[regexp -nocase -- {^INPUT$} $direction] || \
      [regexp -nocase -- {^OUTPUT$} $direction] || \
      [regexp -nocase -- {^INOUT$} $direction] || \
      [regexp -nocase -- {^FEEDTHRU$} $direction]} {
    set direction [string tolower $direction]
    return [ppl::get_direction $direction]      
  } else {
    utl::error PPL 28 "$cmd: Invalid pin direction."
  }
}

proc parse_excludes_arg { args_var } {
  set regions {}
  while { $args_var != {} } {
    set arg [lindex $args_var 0]
    if { $arg == "-exclude" } {
      lappend regions [lindex $args_var 1]
      set args_var [lrange $args_var 1 end]
    } else {
      set args_var [lrange $args_var 1 end]
    }
  }

  return $regions
}

proc parse_group_pins_arg { args_var } {
  set pins {}
  while { $args_var != {} } {
    set arg [lindex $args_var 0]
    if { $arg == "-group_pins" } {
      lappend pins [lindex $args_var 1]
      set args_var [lrange $args_var 1 end]
    } else {
      set args_var [lrange $args_var 1 end]
    }
  }

  return $pins
}

proc get_edge_extreme { cmd begin edge } {
  set dbBlock [ord::get_db_block]
  set die_area [$dbBlock getDieArea]
  if {$begin} {
    if {$edge == "top" || $edge == "bottom"} {
      set extreme [$die_area xMin]
    } elseif {$edge == "left" || $edge == "right"} {
      set extreme [$die_area yMin]
    } else {
      utl::error PPL 29 "$cmd: Invalid edge"
    }
  } else {
    if {$edge == "top" || $edge == "bottom"} {
      set extreme [$die_area xMax]
    } elseif {$edge == "left" || $edge == "right"} {
      set extreme [$die_area yMax]
    } else {
      utl::error PPL 30 "$cmd: Invalid edge"
    }
  }
}

proc exclude_intervals { cmd intervals } {
  if { $intervals != {} } {
    foreach interval $intervals {
      ppl::exclude_interval $interval
    }
  }
}

proc parse_layer_name { layer_name } {
  if { ![ord::db_has_tech] } {
    utl::error PPL 50 "No technology has been read."
  }
  set tech [ord::get_db_tech]
  set tech_layer [$tech findLayer $layer_name]
  if { $tech_layer == "NULL" } {
    utl::error PPL 51 "Layer $layer_name not found."
  }
  set layer_idx [$tech_layer getRoutingLevel]

  return $layer_idx
}

proc add_pins_to_constraint {cmd names edge begin end edge_name} {
  utl::info PPL 48 "Restrict pins \[$names\] to region [ord::dbu_to_microns $begin]u-[ord::dbu_to_microns $end]u at the $edge_name edge."
  set pin_list [ppl::parse_pin_names $cmd $names]
  ppl::add_names_constraint $pin_list $edge $begin $end
}

proc add_pins_to_top_layer {cmd names llx lly urx ury} {
  set tech [ord::get_db_tech]
  set top_layer [ppl::get_top_layer]
  set top_layer_name [[$tech findRoutingLayer $top_layer] getConstName]
  utl::info PPL 60 "Restrict pins \[$names\] to region ([ord::dbu_to_microns $llx]u, [ord::dbu_to_microns $lly]u)-([ord::dbu_to_microns $urx]u, [ord::dbu_to_microns $urx]u) at routing layer $top_layer_name."
  set pin_list [ppl::parse_pin_names $cmd $names]
  ppl::add_top_layer_constraint $pin_list $llx $lly $urx $ury
}

proc parse_pin_names {cmd names} {
  set dbBlock [ord::get_db_block]
  set pin_list {}
  foreach pin [get_ports $names] {
    lappend pin_list [sta::sta_to_db_port $pin]
  }

  if {[llength $pin_list] == 0} {
    utl::error PPL 61 "Pins for $cmd command were not found"
  }

  return $pin_list
}

# ppl namespace end
}
