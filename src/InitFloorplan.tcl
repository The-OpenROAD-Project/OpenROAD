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
  -units def_units\
    [-die_area {lx ly ux uy}]\
    [-core_area {lx ly ux uy}]\
    [-site site_name]\
    [-tracks tracks_file]\
    [-auto_place_pins]}

proc initialize_floorplan { args } {
  parse_key_args "initialize_floorplan" args \
    keys {-units -die_area -core_area -site -tracks \
	  -pin_layer pin_layer} \
    flags {-auto_place_pins}

  if [info exists keys(-die_area)] {
    set die_area $keys(-die_area)
    if { [llength $die_area] != 4 } {
      sta_error "-die_area is a list of 4 coordinates."
    }
    lassign $die_area die_lx die_ly die_ux die_uy
    check_positive_float "-die_area" $die_lx
    check_positive_float "-die_area" $die_ly
    check_positive_float "-die_area" $die_ux
    check_positive_float "-die_area" $die_uy
    } else {
    sta_error "no -core_area specified."
    }

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
  } else {
    sta_error "no -core_area specified."
  }

  set site_name ""
  if [info exists keys(-site)] {
    set site_name $keys(-site)
  }

  set tracks_file ""
  if { [info exists keys(-tracks)] } {
    set tracks_file $keys(-tracks)
  }

  set auto_place_pins [info exists flags(-auto_place_pins)]
  set pin_layer ""
  if { $auto_place_pins } {
    if { [info exists keys(-pin_layer)] } {
      set pin_layer $keys(-pin_layer)
    }
  }

  # convert die/core coordinates to meters.
  init_floorplan_cmd $site_name $tracks_file \
    $auto_place_pins $pin_layer \
    [distance_ui_sta $die_lx] [distance_ui_sta $die_ly] \
    [distance_ui_sta $die_ux] [distance_ui_sta $die_uy] \
    [distance_ui_sta $core_lx] [distance_ui_sta $core_ly] \
    [distance_ui_sta $core_ux] [distance_ui_sta $core_uy]
}


# sta namespace end
}
