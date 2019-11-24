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

namespace eval sta {

define_cmd_args "initialize_floorplan" {
  [-utilization util]\
    [-aspect_ratio ratio]\
    [-core_space space]\
    [-die_area {lx ly ux uy}]\
    [-core_area {lx ly ux uy}]\
    [-site site_name]\
    [-tracks tracks_file]}

proc initialize_floorplan { args } {
  parse_key_args "initialize_floorplan" args \
    keys {-utilization -aspect_ratio -core_space \
	    -die_area -core_area -site -tracks} \
    flags {}

  set site_name ""
  if [info exists keys(-site)] {
    set site_name $keys(-site)
  } else {
    puts "Warning: use -site to add placement rows."
  }

  set tracks_file ""
  if { [info exists keys(-tracks)] } {
    set tracks_file $keys(-tracks)
  }

  if [info exists keys(-utilization)] {
    set util $keys(-utilization)
    check_positive_float "-utilization" $util
    if { $util > 100 } {
      sta_error "-utilization must be from 0% to 100%"
    }
    set util [expr $util / 100.0]
    if [info exists keys(-core_space)] {
      set core_sp $keys(-core_space)
      check_positive_float "-core_space" $core_sp
    } else {
      set core_sp 0.0
    }
    if [info exists keys(-aspect_ratio)] {
      set aspect_ratio $keys(-aspect_ratio)
      check_positive_float "-aspect_ratio" $aspect_ratio
      if { $aspect_ratio > 1.0 } {
	sta_error "-aspect_ratio must be from 0.0 to 1.0"
      }
    } else {
      set aspect_ratio 1.0
    }
    init_floorplan_util $util $aspect_ratio [distance_ui_sta $core_sp] \
      $site_name $tracks_file
  } elseif [info exists keys(-die_area)] {
    set die_area $keys(-die_area)
    if { [llength $die_area] != 4 } {
      sta_error "-die_area is a list of 4 coordinates."
    }
    lassign $die_area die_lx die_ly die_ux die_uy
    check_positive_float "-die_area" $die_lx
    check_positive_float "-die_area" $die_ly
    check_positive_float "-die_area" $die_ux
    check_positive_float "-die_area" $die_uy

    if [info exists keys(-core_area)] {
      set core_area $keys(-core_area)
      if { [llength $core_area] != 4 } {
	sta_error "-core_area is a list of 4 coordinates."
      }
      lassign $core_area core_lx core_ly core_ux core_uy
      check_positive_float "-core_area" $core_lx
      check_positive_float "-core_area" $core_ly
      check_positive_float "-core_area" $core_ux
      check_positive_float "-core_area" $core_uy

      # convert die/core coordinates to meters.
      init_floorplan_core [distance_ui_sta $die_lx] [distance_ui_sta $die_ly] \
	[distance_ui_sta $die_ux] [distance_ui_sta $die_uy] \
	[distance_ui_sta $core_lx] [distance_ui_sta $core_ly] \
	[distance_ui_sta $core_ux] [distance_ui_sta $core_uy] \
	$site_name $tracks_file
    } else {
      sta_error "no -core_area specified."
    }
  } else {
    sta_error "no -utilization or -die_area specified."
  }
}

define_cmd_args "auto_place_pins" {pin_layer}

proc auto_place_pins { pin_layer } {
  auto_place_pins_cmd $pin_layer
}

# sta namespace end
}
