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


sta::define_cmd_args "set_io_pin_constraint" {[-direction direction] \
                                              [-pin_names names] \
                                              [-region region] \
                                              [-names names]}

proc set_io_pin_constraint { args } {
  sta::parse_key_args "set_io_pin_constraint" args \
  keys {-direction -pin_names -region -names}

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
  }

  if {[info exists keys(-direction)] && [info exists keys(-pin_names)] && \
      [info exists keys(-names)]} {
    utl::error PPL 16 "Both -direction and -pin_names constraints not allowed.."
  }

  if [info exists keys(-direction)] {
    set direction $keys(-direction)
    set dir [ppl::parse_direction "set_io_pin_constraint" $direction]
    utl::report "Restrict $direction pins to region [ord::dbu_to_microns $begin]u-[ord::dbu_to_microns $end]u, in the $edge edge."
    ppl::add_direction_constraint $dir $edge_ $begin $end
  }

  if [info exists keys(-names)] {
    utl::warn PPL 44 "-names option is deprecated. Use -pin_names instead."
    set names $keys(-names)
    ppl::add_pins_to_constraint "set_io_pin_constraint" $names $edge_ $begin $end $edge
  }

  if [info exists keys(-pin_names)] {
    set names $keys(-pin_names)
    ppl::add_pins_to_constraint "set_io_pin_constraint" $names $edge_ $begin $end $edge
  }
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

proc io_placer { args } {
  utl::warn PPL 14 "io_placer command is deprecated. Use place_pins instead."
  [eval place_pins $args]
}

proc place_pins { args } {
  set regions [ppl::parse_excludes_arg $args]
  set pin_groups [ppl::parse_group_pins_arg $args]
  sta::parse_key_args "place_pins" args \
  keys {-hor_layers -ver_layers -random_seed -corner_avoidance -min_distance -exclude -group_pins} \
  flags {-random}

  set dbTech [ord::get_db_tech]
  if { $dbTech == "NULL" } {
    utl::error PPL 31 "no technology found."
  }

  set dbBlock [ord::get_db_block]
  if { $dbBlock == "NULL" } {
    utl::error PPL 32 "no block found."
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
    ppl::set_corner_avoidance $distance
  } else {
    utl::report "Using ${distance}u default distance from corners."
    ppl::set_corner_avoidance $distance
  }

  set min_dist 2
  if [info exists keys(-min_distance)] {
    set min_dist $keys(-min_distance)
    ppl::set_min_distance $min_dist
  } else {
    utl::report "Using $min_dist tracks default min distance between IO pins."
    ppl::set_min_distance $min_dist
  }

  set bterms_cnt [llength [$dbBlock getBTerms]]

  if { $bterms_cnt == 0 } {
    utl::error PPL 19 "Design without pins"
  }


  set num_tracks_y 0
  foreach hor_layer $hor_layers {
    if { ![ord::db_layer_has_hor_tracks $hor_layer] } {
      utl::error PPL 21 "Horizontal routing tracks not found for layer $hor_layer."
    }

    set h_tech_layer [$dbTech findRoutingLayer $hor_layer]
    if { [$h_tech_layer getDirection] != "HORIZONTAL" } {
      utl::warn PPL 45 "Layer $hor_layer preferred direction is not horizontal."
    }

    set hor_track_grid [$dbBlock findTrackGrid $h_tech_layer]
    
    set num_tracks_y [expr $num_tracks_y+[llength [$hor_track_grid getGridY]]]
  }

  set num_tracks_x 0
  foreach ver_layer $ver_layers {
    if { ![ord::db_layer_has_ver_tracks $ver_layer] } {
      utl::error PPL 23 "Vertical routing tracks not found for layer $ver_layer."
    }

    set v_tech_layer [$dbTech findRoutingLayer $ver_layer]
    if { [$v_tech_layer getDirection] != "VERTICAL" } {
      utl::warn PPL 46 "Layer $ver_layer preferred direction is not vertical."
    }

    set ver_track_grid [$dbBlock findTrackGrid $v_tech_layer]

    set num_tracks_x [expr $num_tracks_x+[llength [$ver_track_grid getGridX]]]
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
        utl::error PPL 26 "-exclude: invalid syntax in $region. use (top|bottom|left|right):interval."
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

  foreach hor_layer $hor_layers {
    ppl::add_hor_layer $hor_layer 
  }

  foreach ver_layer $ver_layers {
    ppl::add_ver_layer $ver_layer
  }
  
  ppl::run_io_placement [info exists flags(-random)]
}

namespace eval ppl {

proc parse_edge { cmd edge } {
  if {$edge != "top" && $edge != "bottom" && \
      $edge != "left" && $edge != "right"} {
    utl::error PPL 27 "$cmd: $edge is an invalid edge. use top, bottom, left or right."
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

proc add_pins_to_constraint {cmd names edge begin end edge_name} {
  set dbBlock [ord::get_db_block]
  set pin_list {}
  foreach pin_name $names {
    set db_bterm [$dbBlock findBTerm $pin_name]
    if { $db_bterm != "NULL" } {
      utl::report "Restrict I/O pin $pin_name to region [ord::dbu_to_microns $begin]u-[ord::dbu_to_microns $end]u the $edge_name edge."
      lappend pin_list $db_bterm
    } else {
      utl::warn PPL 47 "Pin $pin_name not found in constraint"
    }
  }
  ppl::add_names_constraint $pin_list $edge $begin $end
}

# ppl namespace end
}
