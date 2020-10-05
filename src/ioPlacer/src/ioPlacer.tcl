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

  if { [info exists flags(-random)] } {
    ioPlacer::set_random_mode 2
  }

  set seed 42
  if [info exists keys(-random_seed)] {
    set seed $keys(-random_seed)
  }
  ioPlacer::set_rand_seed $seed

  set hor_layer 3
  if [info exists keys(-hor_layer)] {
    set hor_layer $keys(-hor_layer)
    ioPlacer::set_hor_metal_layer $hor_layer
  } else {
    puts "Warning: use -hor_layer to set the horizontal layer."
  }       
  
  set ver_layer 2
  if [info exists keys(-ver_layer)] {
    set ver_layer $keys(-ver_layer)
    ioPlacer::set_ver_metal_layer $ver_layer
  } else {
    puts "Warning: use -ver_layer to set the vertical layer."
  }

  set offset 5
  if [info exists keys(-boundaries_offset)] {
    set offset $keys(-boundaries_offset)
    ioPlacer::set_boundaries_offset $offset
  } else {
    puts "Warning: using the default boundaries offset ($offset microns)"
    ioPlacer::set_boundaries_offset $offset
  }

  set min_dist 2
  if [info exists keys(-min_distance)] {
    set min_dist $keys(-min_distance)
    ioPlacer::set_min_distance $min_dist
  } else {
    puts "Warning: using the default min distance between IO pins ($min_dist tracks)"
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
        set edge_idx [ioPlacer::parse_edge "-exclude" $edge]

        if [regexp -all {([0-9]+[.]*[0-9]*|[*]+)-([0-9]+[.]*[0-9]*|[*]+)} $interval - begin end] {
          if {$begin == {*}} {
            set begin [ioPlacer::get_edge_extreme 1 $edge_idx]
          }
          if {$end == {*}} {
            set end [ioPlacer::get_edge_extreme 0 $edge_idx]
          }
          set begin [expr { int($begin * $lef_units) }]
          set end [expr { int($end * $lef_units) }]

          ioPlacer::exclude_interval $edge_idx $begin $end
        } elseif {$interval == {*}} {
          set begin [ioPlacer::get_edge_extreme 1 $edge_idx]
          set end [ioPlacer::get_edge_extreme 0 $edge_idx]

          ioPlacer::exclude_interval $edge_idx $begin $end
        }
      }
    }
  }
  
  ioPlacer::run_io_placement 
}

namespace eval ioPlacer {

proc parse_edge { cmd edge } {
  if {$edge == "top"} {
    return 0
  } elseif {$edge == "bottom"} {
    return 1
  } elseif {$edge == "left"} {
    return 2
  } elseif {$edge == "right"} {
    return 3
  } else {
    ord::error "$cmd: Invalid edge"
  }
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

proc get_edge_extreme { begin edge } {
  set dbBlock [ord::get_db_block]
  set die_area [$dbBlock getDieArea]
  if {$begin} {
    if {$edge <= 1} {
      set extreme [$die_area xMin]
    } elseif {$edge >= 2} {
      set extreme [$die_area yMin]
    }
  } else {
    if {$edge <= 1} {
      set extreme [$die_area xMax]
    } elseif {$edge >= 2} {
      set extreme [$die_area yMax]
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
