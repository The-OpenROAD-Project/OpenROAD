#BSD 3-Clause License
#
#Copyright (c) 2023, The Regents of the University of California
#All rights reserved.
#
#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are met:
#
#1. Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
#2. Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
#3. Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

sta::define_cmd_args "make_io_bump_array" {-bump master \
                                           -origin {x y} \
                                           -rows rows \
                                           -columns columns \
                                           -pitch {x y} \
                                           [-prefix prefix]}

proc make_io_bump_array { args } {
  sta::parse_key_args "make_io_bump_array" args \
    keys {-bump -origin -rows -columns -pitch -prefix} \
    flags {}

  sta::check_argc_eq0 "make_io_bump_array" $args

  pad::assert_required make_io_bump_array -bump
  set master [pad::find_master $keys(-bump)]
  pad::assert_required make_io_bump_array -origin
  set origin $keys(-origin)
  pad::assert_required make_io_bump_array -rows
  set rows $keys(-rows)
  pad::assert_required make_io_bump_array -columns
  set columns $keys(-columns)
  pad::assert_required make_io_bump_array -pitch
  set pitch $keys(-pitch)

  set cmd_args []
  if { [info exists keys(-prefix)] } {
    lappend cmd_args $keys(-prefix)
  }

  if { [llength $origin] != 2 } {
    utl::error PAD 17 "-origin must be specified as {x y}"
  }

  if { [llength $pitch] == 1 } {
    lappend pitch $pitch
  } elseif { [llength $pitch] != 2 } {
    utl::error PAD 38 "-pitch must be specified as {deltax deltay} or {delta}"
  }

  pad::make_bump_array \
    $master \
    [ord::microns_to_dbu [lindex $origin 0]] \
    [ord::microns_to_dbu [lindex $origin 1]] \
    $rows \
    $columns \
    [ord::microns_to_dbu [lindex $pitch 0]] \
    [ord::microns_to_dbu [lindex $pitch 1]] \
    {*}$cmd_args
}

sta::define_cmd_args "remove_io_bump_array" {-bump master}

proc remove_io_bump_array { args } {
  sta::parse_key_args "remove_io_bump_array" args \
    keys {-bump} \
    flags {}

  sta::check_argc_eq0 "remove_io_bump_array" $args

  pad::assert_required remove_io_bump_array -bump
  set master [pad::find_master $keys(-bump)]
  pad::remove_bump_array $master
}

sta::define_cmd_args "remove_io_bump" {inst}

proc remove_io_bump { args } {
  sta::parse_key_args "remove_io_bump" args \
    keys {} \
    flags {}

  sta::check_argc_eq1 "remove_io_bump" $args

  pad::remove_bump [pad::find_instance [lindex $args 0]]
}

sta::define_cmd_args "assign_io_bump" {-net net \
                                       [-terminal terminal] \
                                       [-dont_route] \
                                       inst}

proc assign_io_bump { args } {
  sta::parse_key_args "assign_io_bump" args \
    keys {-net -terminal} \
    flags {-dont_route}

  sta::check_argc_eq1 "assign_io_bump" $args

  set terminal NULL
  if { [info exists keys(-terminal)] } {
    set terminal [[ord::get_db_block] findITerm $keys(-terminal)]

    if { $terminal == "NULL" } {
      utl::error PAD 113 "Unable to find $keys(-terminal)"
    }
  }

  set dont_route [info exists flags(-dont_route)]
  if { $dont_route && $terminal != "NULL" } {
    utl::error PAD 114 "-dont_route and -terminal cannot be used together"
  }

  pad::assert_required assign_io_bump -net
  pad::assign_net_to_bump \
    [pad::find_instance [lindex $args 0]] \
    [pad::find_net $keys(-net)] \
    $terminal \
    $dont_route
}

#####

sta::define_cmd_args "make_io_sites" {-horizontal_site site \
                                      -vertical_site site \
                                      -corner_site site \
                                      -offset offset \
                                      [-rotation_horizontal rotation] \
                                      [-rotation_vertical rotation] \
                                      [-rotation_corner rotation] \
                                      [-ring_index index]
} ;# checker off

proc make_io_sites { args } {
  sta::parse_key_args "make_io_sites" args \
    keys {-horizontal_site -vertical_site -corner_site -offset -rotation \
      -rotation_horizontal -rotation_vertical -rotation_corner -ring_index} \
    flags {} ;# checker off

  sta::check_argc_eq0 "make_io_sites" $args
  set index -1
  if { [info exists keys(-ring_index)] } {
    set index $keys(-ring_index)
  }
  set rotation_hor "R0"
  set rotation_ver "R0"
  set rotation_cor "R0"
  if { [info exists keys(-rotation)] } {
    utl::warn PAD 112 "Use of -rotation is deprecated"
    set rotation_hor $keys(-rotation)
    set rotation_ver $keys(-rotation)
    set rotation_cor $keys(-rotation)
  }
  if { [info exists keys(-rotation_horizontal)] } {
    set rotation_hor $keys(-rotation_horizontal)
  }
  if { [info exists keys(-rotation_vertical)] } {
    set rotation_ver $keys(-rotation_vertical)
  }
  if { [info exists keys(-rotation_corner)] } {
    set rotation_cor $keys(-rotation_corner)
  }

  pad::assert_required make_io_sites -horizontal_site
  pad::assert_required make_io_sites -vertical_site
  pad::assert_required make_io_sites -corner_site
  pad::assert_required make_io_sites -offset
  set offset [ord::microns_to_dbu $keys(-offset)]
  pad::make_io_row \
    [pad::find_site $keys(-horizontal_site)] \
    [pad::find_site $keys(-vertical_site)] \
    [pad::find_site $keys(-corner_site)] \
    $offset \
    $offset \
    $offset \
    $offset \
    $rotation_hor \
    $rotation_ver \
    $rotation_cor \
    $index
}

sta::define_cmd_args "remove_io_rows" {}

proc remove_io_rows { args } {
  sta::parse_key_args "remove_io_rows" args \
    keys {} \
    flags {}

  sta::check_argc_eq0 "remove_io_rows" $args

  pad::remove_io_rows
}

sta::define_cmd_args "place_corners" {[-ring_index index] \
                                      master}

proc place_corners { args } {
  sta::parse_key_args "place_corners" args \
    keys {-ring_index} flags {}

  sta::check_argc_eq1 "place_corners" $args

  set master [pad::find_master [lindex $args 0]]
  set index -1
  if { [info exists keys(-ring_index)] } {
    set index $keys(-ring_index)
  }

  pad::place_corner $master $index
}

sta::define_cmd_args "place_pad" {[-master master] \
                                  -row row_name \
                                  -location x_or_y_offset \
                                  -mirror \
                                  name}

proc place_pad { args } {
  sta::parse_key_args "place_pad" args \
    keys {-master -location -row} \
    flags {-mirror}

  sta::check_argc_eq1 "place_pad" $args

  set master "NULL"
  if { [info exists keys(-master)] } {
    set master [pad::find_master $keys(-master)]
  }
  set name [lindex $args 0]
  pad::assert_required place_pad -location
  set offset [ord::microns_to_dbu $keys(-location)]

  pad::assert_required place_pad -row
  pad::place_pad \
    $master \
    $name \
    [pad::get_row $keys(-row)] \
    $offset \
    [info exists flags(-mirror)]
}

sta::define_cmd_args "place_pads" {-row row_name \
                                   pads}

proc place_pads { args } {
  sta::parse_key_args "place_pads" args \
    keys {-row} \
    flags {}

  if { $args == {} } {
    utl::error PAD 39 "place_pads requires a list of instances."
  }

  if { [llength $args] == 1 } {
    set args [lindex $args 0]
  }

  set insts []
  foreach inst $args {
    lappend insts [pad::find_instance $inst]
  }

  pad::assert_required place_pads -row
  pad::place_pads \
    $insts \
    [pad::get_row $keys(-row)]
}

sta::define_cmd_args "place_io_fill" {-row row_name \
                                      [-permit_overlaps masters] \
                                      masters}

proc place_io_fill { args } {
  sta::parse_key_args "place_io_fill" args \
    keys {-row -permit_overlaps} \
    flags {}

  set masters []
  foreach m $args {
    lappend masters [pad::find_master $m]
  }

  set overlap_masters []
  if { [info exists keys(-permit_overlaps)] } {
    foreach m $keys(-permit_overlaps) {
      lappend overlap_masters [pad::find_master $m]
    }
  }

  pad::assert_required place_io_fill -row
  pad::place_filler \
    $masters \
    [pad::get_row $keys(-row)] \
    $overlap_masters
}

sta::define_cmd_args "connect_by_abutment" {}

proc connect_by_abutment { args } {
  sta::parse_key_args "connect_by_abutment" args \
    keys {} \
    flags {}

  sta::check_argc_eq0 "connect_by_abutment" $args

  pad::connect_by_abutment
}

sta::define_cmd_args "place_bondpad" {-bond master \
                                      [-offset {x y}] \
                                      [-rotation rotation] \
                                      ioinsts}

proc place_bondpad { args } {
  sta::parse_key_args "place_bondpad" args \
    keys {-bond -offset -rotation} \
    flags {}

  set insts []
  foreach inst [get_cells {*}$args] {
    lappend insts [sta::sta_to_db_inst $inst]
  }
  if { [llength $insts] == 0 } {
    utl::error PAD 117 "No instances matched $args"
  }
  pad::assert_required place_bondpad -bond
  set master [pad::find_master $keys(-bond)]

  set rotation R0
  if { [info exists keys(-rotation)] } {
    set rotation $keys(-rotation)
  }
  set offset_x 0
  set offset_y 0
  if { [info exists keys(-offset)] } {
    set offset_x [lindex $keys(-offset) 0]
    set offset_y [lindex $keys(-offset) 1]
  }
  set offset_x [ord::microns_to_dbu $offset_x]
  set offset_y [ord::microns_to_dbu $offset_y]

  pad::place_bondpads \
    $master \
    $insts \
    $rotation \
    $offset_x \
    $offset_y
}

sta::define_cmd_args "place_io_terminals" {inst_terms
                                        [-allow_non_top_layer]}

proc place_io_terminals { args } {
  sta::parse_key_args "place_io_terminals" args \
    keys {} \
    flags {-allow_non_top_layer}

  set iterms []
  foreach pin [get_pins {*}$args] {
    lappend iterms [sta::sta_to_db_pin $pin]
  }

  pad::place_terminals $iterms [info exists flags(-allow_non_top_layer)]
}

sta::define_cmd_args "make_fake_io_site" {-name name \
                                          -width width \
                                          -height height}

proc make_fake_io_site { args } {
  sta::parse_key_args "make_fake_io_site" args \
    keys {-name -width -height} \
    flags {}

  sta::check_argc_eq0 "make_fake_io_site" $args

  pad::assert_required make_fake_io_site -name
  pad::assert_required make_fake_io_site -width
  pad::assert_required make_fake_io_site -height
  pad::make_fake_site \
    $keys(-name) \
    [ord::microns_to_dbu $keys(-width)] \
    [ord::microns_to_dbu $keys(-height)]
}

#####

sta::define_cmd_args "rdl_route" {-layer layer \
                                  [-bump_via access_via] \
                                  [-pad_via access_via] \
                                  [-width width] \
                                  [-spacing spacing] \
                                  [-turn_penalty penalty] \
                                  [-allow45] \
                                  [-max_iterations iterations] \
                                  nets}

proc rdl_route { args } {
  sta::parse_key_args "rdl_route" args \
    keys {-layer -width -spacing -bump_via -pad_via -turn_penalty -max_iterations} \
    flags {-allow45}

  if { [llength $args] == 1 } {
    set args [lindex $args 0]
  }

  sta::parse_port_net_args $args sta_ports sta_nets
  set nets []
  foreach net $sta_nets {
    lappend [sta::sta_to_db_net $net]
  }
  foreach port $sta_ports {
    set bterm [sta::sta_to_db_port $port]
    set net [$bterm getNet]
    if { $net != "NULL" } {
      lappend nets $net
    }
  }

  if { [llength $nets] == 0 } {
    utl::error PAD 42 "No nets found to route"
  }

  pad::assert_required rdl_route -layer
  set layer [[ord::get_db_tech] findLayer $keys(-layer)]
  if { $layer == "NULL" } {
    utl::error PAD 105 "Unable to find layer: $keys(-layer)"
  }
  set bump_via "NULL"
  if { [info exists keys(-bump_via)] } {
    set bump_via [[ord::get_db_tech] findVia $keys(-bump_via)]
    if { $bump_via == "NULL" } {
      utl::error PAD 107 "Unable to find techvia: $keys(-bump_via)"
    }
  }
  set pad_via "NULL"
  if { [info exists keys(-pad_via)] } {
    set pad_via [[ord::get_db_tech] findVia $keys(-pad_via)]
    if { $pad_via == "NULL" } {
      utl::error PAD 108 "Unable to find techvia: $keys(-pad_via)"
    }
  }

  set width 0
  if { [info exists keys(-width)] } {
    sta::check_positive_float "-width" $keys(-width)
    set width [ord::microns_to_dbu $keys(-width)]
  }
  set spacing 0
  if { [info exists keys(-spacing)] } {
    sta::check_positive_float "-spacing" $keys(-spacing)
    set spacing [ord::microns_to_dbu $keys(-spacing)]
  }

  set penalty 2.0
  if { [info exists keys(-turn_penalty)] } {
    set penalty $keys(-turn_penalty)
  }
  sta::check_positive_float "-turn_penalty" $penalty

  set max_iterations 10
  if { [info exists keys(-max_iterations)] } {
    set max_iterations $keys(-max_iterations)
  }
  sta::check_positive_int "-max_iterations" $max_iterations

  pad::route_rdl $layer \
    $bump_via $pad_via \
    $nets \
    $width $spacing \
    [info exists flags(-allow45)] \
    $penalty \
    $max_iterations
}

namespace eval pad {
proc find_site { name } {
  set site "NULL"

  foreach lib [[ord::get_db] getLibs] {
    set site [$lib findSite $name]
    if { $site != "NULL" } {
      return $site
    }
  }

  if { $site == "NULL" } {
    utl::error PAD 100 "Unable to find site: $name"
  }
  return $site
}

proc find_master { name } {
  set master [[ord::get_db] findMaster $name]
  if { $master == "NULL" } {
    utl::error PAD 101 "Unable to find master: $name"
  }
  return $master
}

proc find_instance { name } {
  set inst [[ord::get_db_block] findInst $name]
  if { $inst == "NULL" } {
    utl::error PAD 102 "Unable to find instance: $name"
  }
  return $inst
}

proc find_net { name } {
  set net [[ord::get_db_block] findNet $name]
  if { $net == "NULL" } {
    utl::error PAD 103 "Unable to find net: $name"
  }
  return $net
}

proc assert_required { cmd arg } {
  upvar keys keys
  if { ![info exists keys($arg)] } {
    utl::error PAD 104 "$arg is required for $cmd"
  }
}

proc connect_iterm { inst_name iterm_name net_name } {
  set block [ord::get_db_block]
  set inst [$block findInst $inst_name]
  if { $inst == "NULL" } {
    utl::error PAD 109 "Unable to find instance: $inst_name"
  }

  set iterm [$inst findITerm $iterm_name]
  if { $iterm == "NULL" } {
    utl::error PAD 110 "Unable to find iterm: $iterm_name of $inst_name"
  }

  set net [$block findNet $net_name]
  if { $net == "NULL" } {
    utl::error PAD 111 "Unable to find net: $net_name"
  }

  $iterm connect $net
}

proc convert_tcl { } {
  set cmds []
  set cmds_assign []

  set rows 0
  if { [dict exists $ICeWall::library bump array_size rows] } {
    set rows [dict get $ICeWall::library bump array_size rows]
  }

  foreach cell "[dict keys [dict get $ICeWall::footprint padcell]]" {
    set param [dict get $ICeWall::footprint padcell $cell]

    set origin [dict get $param cell scaled_origin]
    set side [dict get $param side]
    if { $side == "top" } {
      set side "IO_NORTH"
      set location [ord::dbu_to_microns [lindex $origin 1]]
    } elseif { $side == "left" } {
      set side "IO_WEST"
      set location [ord::dbu_to_microns [lindex $origin 3]]
    } elseif { $side == "bottom" } {
      set side "IO_SOUTH"
      set location [ord::dbu_to_microns [lindex $origin 1]]
    } elseif { $side == "right" } {
      set side "IO_EAST"
      set location [ord::dbu_to_microns [lindex $origin 3]]
    }

    set inst [dict get $param inst]
    set inst_name [$inst getName]
    set master [[$inst getMaster] getName]

    lappend cmds "place_pad -master $master -row $side -location $location {$inst_name}"

    set icewall_type [dict get $param type]
    set icewall_cell [dict get $ICeWall::library types $icewall_type]
    if { ![dict exists $ICeWall::library cells $icewall_cell pad_pin_name] } {
      continue
    }
    set pad_term [dict get $ICeWall::library cells $icewall_cell pad_pin_name]

    set iterm [$inst findITerm $pad_term]
    if { $iterm != "NULL" } {
      set net [$iterm getNet]
      if { $net != "NULL" } {
        lappend cmds "pad::connect_iterm {$inst_name} [[$iterm getMTerm] getName] [$net getName]"

        if { [dict exists $param bump] } {
          set bump [dict get $param bump]
          set row [expr $rows - [dict get $bump row]]
          set col [expr [dict get $bump col] - 1]
          lappend cmds_assign "assign_io_bump -net {[$net getName]} BUMP_${col}_${row}"
        }
      }
    }
  }

  puts "######## Place Pads ########"
  foreach c $cmds {
    puts $c
  }
  puts "######## Assign Bumps ########"
  foreach c $cmds_assign {
    puts $c
  }
}
}
