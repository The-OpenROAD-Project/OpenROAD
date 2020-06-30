############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, James Cherry, Parallax Software, Inc.
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
##
############################################################################

sta::define_cmd_args "initialize_floorplan" {[-utilization util]\
					       [-aspect_ratio ratio]\
					       [-core_space space | {bottom top left right}]\
					       [-die_area {lx ly ux uy}]\
					       [-core_area {lx ly ux uy}]\
					       [-site site_name]\
					       [-tracks tracks_file]}

proc initialize_floorplan { args } {
  sta::parse_key_args "initialize_floorplan" args \
    keys {-utilization -aspect_ratio -core_space \
	    -die_area -core_area -site -tracks} \
    flags {}

  ord::ensure_units_initialized

  set site_name ""
  if [info exists keys(-site)] {
    set site_name $keys(-site)
  } else {
    ord::warn "use -site to add placement rows."
  }

  set tracks_file ""
  if { [info exists keys(-tracks)] } {
    set tracks_file $keys(-tracks)
  }

  sta::check_argc_eq0 "initialize_floorplan" $args
  if [info exists keys(-utilization)] {
    set util $keys(-utilization)
    sta::check_positive_float "-utilization" $util
    if { $util > 100 } {
      ord::error "-utilization must be from 0% to 100%"
    }
    set util [expr $util / 100.0]
    if [info exists keys(-core_space)] {
      set core_sp $keys(-core_space)
      if { [llength $core_sp] == 1} {
        sta::check_positive_float "-core_space" $core_sp
        set core_sp_bottom $core_sp
        set core_sp_top $core_sp
        set core_sp_left $core_sp
        set core_sp_right $core_sp
      } elseif { [llength $core_sp] == 4} {
        lassign $core_sp core_sp_bottom core_sp_top core_sp_left core_sp_right
        sta::check_positive_float "-core_space" $core_sp_bottom
        sta::check_positive_float "-core_space" $core_sp_top
        sta::check_positive_float "-core_space" $core_sp_left
        sta::check_positive_float "-core_space" $core_sp_right
      } else {
        ord::error "-core_space is either a list of 4 margins or one value for all margins."
      }
    } else {
      set core_sp_bottom 0.0
      set core_sp_top 0.0
      set core_sp_left 0.0
      set core_sp_right 0.0
    }
    if [info exists keys(-aspect_ratio)] {
      set aspect_ratio $keys(-aspect_ratio)
      sta::check_positive_float "-aspect_ratio" $aspect_ratio
      if { $aspect_ratio > 1.0 } {
	ord::error "-aspect_ratio must be from 0.0 to 1.0"
      }
    } else {
      set aspect_ratio 1.0
    }
    ord::init_floorplan_util $util $aspect_ratio \
      [sta::distance_ui_sta $core_sp_bottom] \
      [sta::distance_ui_sta $core_sp_top] \
      [sta::distance_ui_sta $core_sp_left] \
      [sta::distance_ui_sta $core_sp_right] \
      $site_name $tracks_file
  } elseif [info exists keys(-die_area)] {
    set die_area $keys(-die_area)
    if { [llength $die_area] != 4 } {
      ord::error "-die_area is a list of 4 coordinates."
    }
    lassign $die_area die_lx die_ly die_ux die_uy
    sta::check_positive_float "-die_area" $die_lx
    sta::check_positive_float "-die_area" $die_ly
    sta::check_positive_float "-die_area" $die_ux
    sta::check_positive_float "-die_area" $die_uy

    ord::ensure_linked
    if [info exists keys(-core_area)] {
      set core_area $keys(-core_area)
      if { [llength $core_area] != 4 } {
	ord::error "-core_area is a list of 4 coordinates."
      }
      lassign $core_area core_lx core_ly core_ux core_uy
      sta::check_positive_float "-core_area" $core_lx
      sta::check_positive_float "-core_area" $core_ly
      sta::check_positive_float "-core_area" $core_ux
      sta::check_positive_float "-core_area" $core_uy

      # convert die/core coordinates to meters.
      ord::init_floorplan_core \
	[sta::distance_ui_sta $die_lx] [sta::distance_ui_sta $die_ly] \
	[sta::distance_ui_sta $die_ux] [sta::distance_ui_sta $die_uy] \
	[sta::distance_ui_sta $core_lx] [sta::distance_ui_sta $core_ly] \
	[sta::distance_ui_sta $core_ux] [sta::distance_ui_sta $core_uy] \
	$site_name $tracks_file
    } else {
      ord::error "no -core_area specified."
    }
  } else {
    ord::error "no -utilization or -die_area specified."
  }
}

sta::define_cmd_args "auto_place_pins" {pin_layer}

proc auto_place_pins { pin_layer } {
  if { [[ord::get_db_tech] findLayer $pin_layer] != "NULL" } {
    ord::auto_place_pins_cmd $pin_layer
  } else {
    ord::error "layer $pin_layer not found."
  }
}
