###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, The Regents of the University of California
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


sta::define_cmd_args "tapcell" {[-tapcell_master tapcell_master]\
                                [-tap_prefix tap_prefix]\
                                [-endcap_master endcap_master]\
                                [-endcap_prefix endcap_prefix]\
                                [-distance dist]\
                                [-disallow_one_site_gaps]\
                                [-halo_width_x halo_x]\
                                [-halo_width_y halo_y]\
                                [-tap_nwin2_master tap_nwin2_master]\
                                [-tap_nwin3_master tap_nwin3_master]\
                                [-tap_nwout2_master tap_nwout2_master]\
                                [-tap_nwout3_master tap_nwout3_master]\
                                [-tap_nwintie_master tap_nwintie_master]\
                                [-tap_nwouttie_master tap_nwouttie_master]\
                                [-cnrcap_nwin_master cnrcap_nwin_master]\
                                [-cnrcap_nwout_master cnrcap_nwout_master]\
                                [-incnrcap_nwin_master incnrcap_nwin_master]\
                                [-incnrcap_nwout_master incnrcap_nwout_master]\
                                [-tbtie_cpp tbtie_cpp]\
                                [-endcap_cpp endcap_cpp]\
                                [-no_cell_at_top_bottom]\
}

# Main function. It will run tapcell given the correct parameters
proc tapcell { args } {
  sta::parse_key_args "tapcell" args \
    keys {-tapcell_master -endcap_master -endcap_cpp -distance -halo_width_x \
            -halo_width_y -tap_nwin2_master -tap_nwin3_master -tap_nwout2_master \
              -tap_nwout3_master -tap_nwintie_master -tap_nwouttie_master \
              -cnrcap_nwin_master -cnrcap_nwout_master -incnrcap_nwin_master \
              -incnrcap_nwout_master -tbtie_cpp -tap_prefix -endcap_prefix} \
    flags {-no_cell_at_top_bottom -disallow_one_site_gaps}

  sta::check_argc_eq0 "tapcell" $args

  if { [ord::get_db_block] == "NULL" } {
    utl::error TAP 1 "No design block found."
  }

  tap::clear

  if { [info exists keys(-endcap_cpp)] } {
    utl::warn TAP 14 "endcap_cpp option is deprecated."
  }

  set dist -1
  if { [info exists keys(-distance)] } {
    set dist $keys(-distance)
  }

  set halo_y -1
  if { [info exists keys(-halo_width_y)] } {
    set halo_y $keys(-halo_width_y)
  }

  set halo_x -1
  if { [info exists keys(-halo_width_x)] } {
    set halo_x $keys(-halo_width_x)
  }

  set tap_nwin2_master ""
  if { [info exists keys(-tap_nwin2_master)] } {
    set tap_nwin2_master $keys(-tap_nwin2_master)
  }

  set tap_nwin3_master ""
  if { [info exists keys(-tap_nwin3_master)] } {
    set tap_nwin3_master $keys(-tap_nwin3_master)
  }

  set tap_nwout2_master ""
  if { [info exists keys(-tap_nwout2_master)] } {
    set tap_nwout2_master $keys(-tap_nwout2_master)
  }

  set tap_nwout3_master ""
  if { [info exists keys(-tap_nwout3_master)] } {
    set tap_nwout3_master $keys(-tap_nwout3_master)
  }

  set tap_nwintie_master ""
  if { [info exists keys(-tap_nwintie_master)] } {
    set tap_nwintie_master $keys(-tap_nwintie_master)
  }

  set tap_nwouttie_master ""
  if { [info exists keys(-tap_nwouttie_master)] } {
    set tap_nwouttie_master $keys(-tap_nwouttie_master)
  }

  set cnrcap_nwin_master ""
  if { [info exists keys(-cnrcap_nwin_master)] } {
    set cnrcap_nwin_master $keys(-cnrcap_nwin_master)
  }

  set cnrcap_nwout_master ""
  if { [info exists keys(-cnrcap_nwout_master)] } {
    set cnrcap_nwout_master $keys(-cnrcap_nwout_master)
  }

  set incnrcap_nwin_master ""
  if { [info exists keys(-incnrcap_nwin_master)] } {
    set incnrcap_nwin_master $keys(-incnrcap_nwin_master)
  }

  set incnrcap_nwout_master ""
  if { [info exists keys(-incnrcap_nwout_master)] } {
    set incnrcap_nwout_master $keys(-incnrcap_nwout_master)
  }

  if { [info exists keys(-tbtie_cpp)] } {
    utl::warn TAP 15 "tbtie_cpp option is deprecated."
  }

  if {[info exists flags(-no_cell_at_top_bottom)]} {
    utl::warn TAP 16 "no_cell_at_top_bottom option is deprecated."
  }

  set disallow_one_site_gaps [info exists flags(-disallow_one_site_gaps)]
  if { [info exists keys(-tap_prefix)] } {
    tap::set_tap_prefix $keys(-tap_prefix)
  }

  if { [info exists keys(-endcap_prefix)] } {
    tap::set_endcap_prefix $keys(-endcap_prefix)
  }

  set db [ord::get_db]

  set halo_y [ord::microns_to_dbu $halo_y]
  set halo_x [ord::microns_to_dbu $halo_x]
  set dist [ord::microns_to_dbu $dist]

  set tapcell_master "NULL"
  if { [info exists keys(-tapcell_master)] } {
    set tapcell_master_name $keys(-tapcell_master)
    set tapcell_master [$db findMaster $tapcell_master_name]
    if { $tapcell_master == "NULL" } {
      utl::error TAP 10 "Master $tapcell_master_name not found."
    }
  }


  set endcap_master "NULL"
  if { [info exists keys(-endcap_master)] } {
    set endcap_master_name $keys(-endcap_master)
    set endcap_master [$db findMaster $endcap_master_name]
    if { $endcap_master == "NULL" } {
      utl::error TAP 11 "Master $endcap_master_name not found."
    }
  }

  tap::run $endcap_master $halo_x $halo_y $cnrcap_nwin_master \
    $cnrcap_nwout_master $tap_nwintie_master $tap_nwin2_master \
    $tap_nwin3_master $tap_nwouttie_master $tap_nwout2_master \
    $tap_nwout3_master $incnrcap_nwin_master $incnrcap_nwout_master \
    $tapcell_master $dist $disallow_one_site_gaps
}

sta::define_cmd_args "cut_rows" {[-endcap_master endcap_master]\
                                 [-halo_width_x halo_x]\
                                 [-halo_width_y halo_y]
}
proc cut_rows { args } {
  sta::parse_key_args "cut_rows" args \
    keys {-endcap_master -halo_width_x -halo_width_y} flags {}

  sta::check_argc_eq0 "cut_rows" $args

  set halo_x -1
  if { [info exists keys(-halo_width_x)] } {
    set halo_x $keys(-halo_width_x)
  }
  set halo_y -1
  if { [info exists keys(-halo_width_y)] } {
    set halo_y $keys(-halo_width_y)
  }

  set halo_x [ord::microns_to_dbu $halo_x]
  set halo_y [ord::microns_to_dbu $halo_y]

  set endcap_master "NULL"
  if { [info exists keys(-endcap_master)] } {
    set endcap_master_name $keys(-endcap_master)
    set endcap_master [[ord::get_db] findMaster $endcap_master_name]
    if { $endcap_master == "NULL" } {
      utl::error TAP 34 "Master $endcap_master_name not found."
    }
  }

  tap::cut_rows $endcap_master $halo_x $halo_y
}

sta::define_cmd_args "tapcell_ripup" {[-tap_prefix tap_prefix]\
                                      [-endcap_prefix endcap_prefix]
}

# This will remove the tap cells and endcaps to tapcell can be rerun with new parameters
proc tapcell_ripup { args } {
  sta::parse_key_args "tapcell_ripup" args \
    keys {-tap_prefix -endcap_prefix} \
    flags {}

  if { [ord::get_db_block] == "NULL" } {
    utl::error TAP 2 "No design block found."
  }

  sta::check_argc_eq0 "tapcell_ripup" $args

  set tap_prefix "TAP_"
  if { [info exists keys(-tap_prefix)] } {
    set tap_prefix $keys(-tap_prefix)
  }

  set endcap_prefix "PHY_"
  if { [info exists keys(-endcap_prefix)] } {
    set endcap_prefix $keys(-endcap_prefix)
  }

  tap::set_tap_prefix $tap_prefix
  tap::set_endcap_prefix $endcap_prefix

  set taps_removed [tap::remove_cells $tap_prefix]
  utl::info TAP 100 "Removed $taps_removed tapcells."
  set endcaps_removed [tap::remove_cells $endcap_prefix]
  utl::info TAP 101 "Removed $endcaps_removed endcaps."

  # Reset global parameters
  tap::reset
}

sta::define_cmd_args "place_endcaps" {
  # Simplified
  [-corner master]\
  [-edge_corner master]\

  [-endcap masters] \

  [-endcap_horizontal masters] \
  [-endcap_vertical master] \

  [-prefix prefix]

  # Full options
  [-left_top_corner master]\
  [-right_top_corner master]\
  [-left_bottom_corner master]\
  [-right_bottom_corner master]\

  [-left_top_edge master]\
  [-right_top_edge master]\
  [-left_bottom_edge master]\
  [-right_bottom_edge master]\

  [-left_edge master] \
  [-right_edge master] \

  [-top_edge masters] \
  [-bottom_edge masters]
}

proc place_endcaps { args } {
  sta::parse_key_args "place_endcaps" args \
    keys {
      -corner -edge_corner
      -endcap
      -endcap_horizontal -endcap_vertical
      -prefix
      -left_top_corner -right_top_corner -left_bottom_corner -right_bottom_corner
      -left_top_edge -right_top_edge -left_bottom_edge -right_bottom_edge
      -left_edge -right_edge
      -top_edge -bottom_edge} \
    flags {}

  if { [ord::get_db_block] == "NULL" } {
    utl::error TAP 6 "No design block found."
  }

  sta::check_argc_eq0 "place_endcaps" $args

  set prefix "PHY_"
  if { [info exists keys(-prefix)]} {
    set prefix $keys(-prefix)
  }

  set left_top_corner [tap::find_master \
    [tap::parse_endcap_key keys -left_top_corner -corner -corner]]
  set right_top_corner [tap::find_master \
    [tap::parse_endcap_key keys -right_top_corner -corner -corner]]
  set left_bottom_corner [tap::find_master \
    [tap::parse_endcap_key keys -left_bottom_corner -corner -corner]]
  set right_bottom_corner [tap::find_master \
    [tap::parse_endcap_key keys -right_bottom_corner -corner -corner]]

  set left_top_edge [tap::find_master \
    [tap::parse_endcap_key keys -left_top_edge -edge_corner -edge_corner]]
  set right_top_edge [tap::find_master \
    [tap::parse_endcap_key keys -right_top_edge -edge_corner -edge_corner]]
  set left_bottom_edge [tap::find_master \
    [tap::parse_endcap_key keys -left_bottom_edge -edge_corner -edge_corner]]
  set right_bottom_edge [tap::find_master \
    [tap::parse_endcap_key keys -right_bottom_edge -edge_corner -edge_corner]]

  set left_edge [tap::find_master \
    [tap::parse_endcap_key keys -left_edge -endcap_vertical -endcap]]
  set right_edge [tap::find_master \
    [tap::parse_endcap_key keys -right_edge -endcap_vertical -endcap]]

  set top_edge [tap::find_masters \
    [tap::parse_endcap_key keys -top_edge -endcap_horizontal -endcap]]
  set bottom_edge [tap::find_masters \
    [tap::parse_endcap_key keys -bottom_edge -endcap_horizontal -endcap]]

  tap::place_endcaps \
    $left_top_corner \
    $right_top_corner \
    $left_bottom_corner \
    $right_bottom_corner \
    $left_top_edge \
    $right_top_edge \
    $left_bottom_edge \
    $right_bottom_edge \
    $top_edge \
    $bottom_edge \
    $left_edge \
    $right_edge \
    $prefix
}

sta::define_cmd_args "place_tapcells" {
  -master tapcell_master \
  -distance dist
}

proc place_tapcells {args } {

  sta::parse_key_args "place_tapcells" args \
    keys {-master -distance} \
    flags {}

  sta::check_argc_eq0 "place_tapcells" $args

  set dist -1
  if { [info exists keys(-distance)] } {
    set dist [ord::microns_to_dbu $keys(-distance)]
  }
  set master "NULL"
  if { [info exists keys(-master)] } {
    set master [tap::find_master $keys(-master)]
  }

  tap::insert_tapcells $master $dist
}

namespace eval tap {

proc find_master { master } {
  if { $master == "" } {
    return "NULL"
  }
  set m [[ord::get_db] findMaster $master]
  if {$m == "NULL"} {
    utl::error TAP 102 "Unable to find $master"
  }
  return $m
}

proc find_masters { masters } {
  set ms []
  foreach m $masters {
    lappend ms [find_master $m]
  }
  return $ms
}

proc parse_endcap_key { cmdargs key0 key1 key2 {optional true} } {
  upvar keys $cmdargs

  if { [info exists keys($key0) ]} {
    return $keys($key0)
  }
  if { [info exists keys($key1) ]} {
    return $keys($key1)
  }
  if { [info exists keys($key2) ]} {
    return $keys($key2)
  }

  if { !$optional } {
    utl::error TAP 103 "$key0, $key1, or $key2 is required."
  }

  return ""
}

}
