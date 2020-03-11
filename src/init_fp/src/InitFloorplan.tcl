# Copyright (c) 2019, Parallax Software, Inc.
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

sta::define_cmd_args "initialize_floorplan" {[-utilization util]\
					       [-aspect_ratio ratio]\
					       [-core_space space]\
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
      sta::check_positive_float "-core_space" $core_sp
    } else {
      set core_sp 0.0
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
    ord::init_floorplan_util $util $aspect_ratio [sta::distance_ui_sta $core_sp] \
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
