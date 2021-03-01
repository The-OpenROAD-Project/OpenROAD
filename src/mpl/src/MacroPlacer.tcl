#############################################################################
##
## Copyright (c) 2019, OpenROAD
## All rights reserved.
##
## BSD 3-Clause License
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

sta::define_cmd_args "macro_placement" {
  [-halo {vertical_width horizontal_width}] \
    [-channel {vertical_width horizontal_width}]\
    [-fence_region {lx ly ux uy}]\
    [-snap_layer snap_layer_number]\
    [-style corner_max_wl|center_spread]}

proc macro_placement { args } {
  sta::parse_key_args "macro_placement" args \
    keys {-channel -halo -fence_region -snap_layer -style} flags {}

  if { [info exists keys(-halo)] } {
    set halo $keys(-halo)
    if { [llength $halo] != 2 } {
      utl::error "MPL" 92 "-halo is not a list of 2 values."
    }
    lassign $halo halo_x halo_y
    sta::check_positive_float "-halo x" $halo_x
    sta::check_positive_float "-halo y" $halo_y
    mpl::set_halo $halo_x $halo_y
  }

  if { [info exists keys(-channel)] } {
    set channel $keys(-channel)
    if { [llength $channel] != 2 } {
      utl::error "MPL" 93 "-channel is not a list of 2 values."
    }
    lassign $channel channel_x channel_y
    sta::check_positive_float "-channel x" $channel_x
    sta::check_positive_float "-channel y" $channel_y
    mpl::set_channel $channel_x $channel_y
  }

  if { ![ord::db_has_rows] } {
    utl::error "MPL" 89 "No rows found. Use initialize_floorplan to add rows."
  }
  set core [ord::get_db_core]
  set core_lx [ord::dbu_to_microns [$core xMin]]
  set core_ly [ord::dbu_to_microns [$core yMin]]
  set core_ux [ord::dbu_to_microns [$core xMax]]
  set core_uy [ord::dbu_to_microns [$core yMax]]
  
  if { [info exists keys(-fence_region)] } {
    set fence_region $keys(-fence_region)
    if { [llength $fence_region] != 4 } {
      utl::error "MPL" 94 "-fence_region is not a list of 4 values."
    }
    lassign $fence_region lx ly ux uy 
    
    if { $lx < $core_lx || $ly < $core_ly || $ux > $core_ux || $uy > $core_uy } {
      utl::warn "MPL" 85 "fence_region outside of core area. Using core area."
    }
    mpl::set_fence_region $lx $ly $ux $uy
  } else {
    mpl::set_fence_region $core_lx $core_ly $core_ux $core_uy
  }

  set snap_layer 4
  if { [info exists keys(-snap_layer)] } {
    set snap_layer $keys(-snap_layer)
    sta::check_positive_integer "-snap_layer" $snap_layer
  }
  set tech [ord::get_db_tech]
  set layer [$tech findRoutingLayer $snap_layer]
  if { $layer == "NULL" } {
    utl::error "MPL" 95 "snap layer $snap_layer is not a routing layer."
  }
  mpl::set_snap_layer $layer

  set style "corner_max_wl"
  if { [info exists keys(-style)] } {
    set style $keys(-style)
  }
  if { $style == "corner_max_wl" } {
    mpl::place_macros_corner_max_wl
  } elseif { $style == "center_spread" } {
    mpl::place_macros_center_spread
  } else {
    utl::error MPL 96 "Unknown placement style."
  }
}

sta::define_cmd_args "macro_placement_debug" {
    [-partitions]
}

proc macro_placement_debug { args } {
  sta::parse_key_args "macro_placement_debug" args \
      keys {} \
      flags {-partitions}

  set partitions [info exists flags(-partitions)]

  mpl::set_debug_cmd $partitions
}
