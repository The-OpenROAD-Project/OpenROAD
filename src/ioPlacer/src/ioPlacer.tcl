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
                                            [-region region]}

proc set_io_pin_constraint { args } {
  sta::parse_key_args "set_io_pin_constraint" args \
  keys {-direction -region}

  if [info exists keys(-direction)] {
    set direction $keys(-direction)
  }

  if [info exists keys(-region)] {
    set region $keys(-region)
  }

  set dir [ioPlacer::parse_direction "set_io_pin_constraint" $direction]

  if [regexp -all {(top|bottom|left|right):(.+)} $region - edge interval] {
    set edge_ [ioPlacer::parse_edge "-exclude" $edge]

    if [regexp -all {([0-9]+[.]*[0-9]*|[*]+)-([0-9]+[.]*[0-9]*|[*]+)} $interval - begin end] {
      if {$begin == {*}} {
        set begin [ioPlacer::get_edge_extreme "-exclude" 1 $edge]
      }
      if {$end == {*}} {
        set end [ioPlacer::get_edge_extreme "-exclude" 0 $edge]
      }

      set begin [expr { int($begin * $lef_units) }]
      set end [expr { int($end * $lef_units) }]
    } elseif {$interval == {*}} {
      set begin [ioPlacer::get_edge_extreme "-exclude" 1 $edge]
      set end [ioPlacer::get_edge_extreme "-exclude" 0 $edge]
    }
  }

  puts "Restrict $direction pins to region $begin-$end, in the $edge edge"
  ioPlacer::add_direction_restriction $dir $edge_ $begin $end
}

sta::define_cmd_args "io_placer" {[-hor_layer h_layer]        \ 
                                  [-ver_layer v_layer]        \
                                  [-random_seed seed]         \
                       	          [-random]                   \
                                  [-boundaries_offset offset] \
                                  [-min_distance min_dist]    \
                                  [-exclude region]         \
                                 }

sta::define_cmd_alias "place_ios" "io_placer"
sta::define_cmd_alias "place_pins" "io_placer"

proc io_placer { args } {
  sta::parse_key_args "io_placer" args \
  keys {-hor_layer -ver_layer -random_seed -boundaries_offset -min_distance} \
  flags {-random} 0

  set dbTech [ord::get_db_tech]
  if { $dbTech == "NULL" } {
    ord::error "missing dbTech"
  }

  set dbBlock [ord::get_db_block]
  if { $dbBlock == "NULL" } {
    ord::error "missing dbBlock"
  }

  set db [::ord::get_db]
  
  set blockages {}

  foreach inst [$dbBlock getInsts] {
    if { [$inst isBlock] } {
      if { ![$inst isPlaced] } {
        puts "\[ERROR\] Macro [$inst getName] is not placed"
          continue
      }
      lappend blockages $inst
    }
  }

  puts "#Macro blocks found: [llength $blockages]"

  set seed 42
  if [info exists keys(-random_seed)] {
    set seed $keys(-random_seed)
  }
  ioPlacer::set_rand_seed $seed

  if [info exists keys(-hor_layer)] {
    set hor_layer $keys(-hor_layer)
  } else {
    ord::error("-hor_layer is mandatory")
  }       
  
  if [info exists keys(-ver_layer)] {
    set ver_layer $keys(-ver_layer)
  } else {
    ord::error("-ver_layer is mandatory")
  }

  set offset 5
  if [info exists keys(-boundaries_offset)] {
    set offset $keys(-boundaries_offset)
    ioPlacer::set_boundaries_offset $offset
  } else {
    puts "Using ${offset}u default boundaries offset"
    ioPlacer::set_boundaries_offset $offset
  }

  set min_dist 2
  if [info exists keys(-min_distance)] {
    set min_dist $keys(-min_distance)
    ioPlacer::set_min_distance $min_dist
  } else {
    puts "Using $min_dist tracks default min distance between IO pins"
    ioPlacer::set_min_distance $min_dist
  }

  set bterms_cnt [llength [$dbBlock getBTerms]]

  if { $bterms_cnt == 0 } {
    ord::error "Design without pins"
  }

  set hor_track_grid [$dbBlock findTrackGrid [$dbTech findRoutingLayer $hor_layer]]
  set ver_track_grid [$dbBlock findTrackGrid [$dbTech findRoutingLayer $ver_layer]]

  if { $hor_track_grid == "NULL" } {
    ord::error "Horizontal routing layer ($hor_layer) not found"
  }

  if { $ver_track_grid == "NULL" } {
    ord::error "Vertical routing layer ($ver_layer) not found"
  }

  if { ![ord::db_layer_has_hor_tracks $hor_layer] || \
       ![ord::db_layer_has_ver_tracks $ver_layer] } {
    ord::error "missing track structure"
  }

  set num_tracks_x [llength [$ver_track_grid getGridX]]
  set num_tracks_y [llength [$hor_track_grid getGridY]]
  
  set num_slots [expr (2*$num_tracks_x + 2*$num_tracks_y)/$min_dist]

  if { ($bterms_cnt > $num_slots) } {
    ord::error "Number of pins ($bterms_cnt) exceed max possible ($num_slots)"
  }
 
  set arg_error 0
  set regions [ioPlacer::parse_excludes_arg args arg_error]
  if { $regions != {} } {
    set lef_units [$dbTech getLefUnits]
    
    foreach region $regions {
      if [regexp -all {(top|bottom|left|right):(.+)} $region - edge interval] {
        set edge_ [ioPlacer::parse_edge "-exclude" $edge]

        if [regexp -all {([0-9]+[.]*[0-9]*|[*]+)-([0-9]+[.]*[0-9]*|[*]+)} $interval - begin end] {
          if {$begin == {*}} {
            set begin [ioPlacer::get_edge_extreme "-exclude" 1 $edge]
          }
          if {$end == {*}} {
            set end [ioPlacer::get_edge_extreme "-exclude" 0 $edge]
          }
          set begin [expr { int($begin * $lef_units) }]
          set end [expr { int($end * $lef_units) }]

          ioPlacer::exclude_interval $edge_ $begin $end
        } elseif {$interval == {*}} {
          set begin [ioPlacer::get_edge_extreme "-exclude" 1 $edge]
          set end [ioPlacer::get_edge_extreme "-exclude" 0 $edge]

          ioPlacer::exclude_interval $edge_ $begin $end
        }
      }
    }
  }
  
  ioPlacer::run_io_placement $hor_layer $ver_layer [info exists flags(-random)]
}

namespace eval ioPlacer {

proc parse_edge { cmd edge } {
  if {$edge != "top" && $edge != "bottom" && \
      $edge != "left" && $edge != "right"} {
    ord::error "$cmd: Invalid edge"
  }
  return [ioPlacer::get_edge $edge]
}

proc parse_direction { cmd direction } {
  if {$direction != "INPUT" && $direction != "OUTPUT" && \
      $direction != "INOUT" && $direction != "FEEDTHRU"} {
    ord::error "$cmd: Invalid pin direction"
  }
  return [ioPlacer::get_direction $direction]      
}

proc parse_excludes_arg { args_var arg_error_var } {
  upvar 1 $args_var args
  
  set regions {}
  while { $args != {} } {
    set arg [lindex $args 0]
    if { $arg == "-exclude" } {
      lappend regions [lindex $args 1]
      set args [lrange $args 1 end]
    } else {
      set args [lrange $args 1 end]
    }
  }

  return $regions
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
      ord::error "$cmd: Invalid edge"
    }
  } else {
    if {$edge == "top" || $edge == "bottom"} {
      set extreme [$die_area xMax]
    } elseif {$edge == "left" || $edge == "right"} {
      set extreme [$die_area yMax]
    } else {
      ord::error "$cmd: Invalid edge"
    }
  }
}

proc exclude_intervals { cmd intervals } {
  if { $intervals != {} } {
    foreach interval $intervals {
      ioPlacer::exclude_interval $interval
    }
  }
}

# ioPlacer namespace end
}
