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
					       [-additional_sites site_names]\
					       [-site site_name]\
					       [-row_parity NONE|ODD|EVEN]}

proc initialize_floorplan { args } {
  sta::parse_key_args "initialize_floorplan" args \
    keys {-utilization -aspect_ratio -core_space \
	    -die_area -core_area -site -additional_sites -row_parity} \
    flags {}

  sta::check_argc_eq0 "initialize_floorplan" $args

  set site ""
  if { [info exists keys(-site)] } {
    set site [ifp::find_site $keys(-site)]
  } else {
    utl::warn IFP 11 "use -site to add placement rows."
  }

  set additional_sites {}
  if { [info exists keys(-additional_sites)] } {
    foreach sitename $keys(-additional_sites) {
      lappend additional_sites [ifp::find_site $sitename]
    }
  }

  set row_parity "NONE"
  if { [info exists keys(-row_parity)] } {
    set row_parity $keys(-row_parity)
    if { $row_parity != "NONE" && $row_parity != "ODD" && $row_parity != "EVEN" } {
      utl::error IFP 12 "-row_parity must be NONE, ODD or EVEN"
    }
  }

  sta::check_argc_eq0 "initialize_floorplan" $args
  if { [info exists keys(-utilization)] } {
    set util $keys(-utilization)
    if { [info exists keys(-core_space)] } {
      set core_sp $keys(-core_space)
      if { [llength $core_sp] == 1 } {
        sta::check_positive_float "-core_space" $core_sp
        set core_sp_bottom $core_sp
        set core_sp_top $core_sp
        set core_sp_left $core_sp
        set core_sp_right $core_sp
      } elseif { [llength $core_sp] == 4 } {
        lassign $core_sp core_sp_bottom core_sp_top core_sp_left core_sp_right
        sta::check_positive_float "-core_space" $core_sp_bottom
        sta::check_positive_float "-core_space" $core_sp_top
        sta::check_positive_float "-core_space" $core_sp_left
        sta::check_positive_float "-core_space" $core_sp_right
      } else {
        utl::error IFP 13 "-core_space is either a list of 4 margins or one value for all margins."
      }
    } else {
      set core_sp_bottom 0.0
      set core_sp_top 0.0
      set core_sp_left 0.0
      set core_sp_right 0.0
    }
    if { [info exists keys(-aspect_ratio)] } {
      set aspect_ratio $keys(-aspect_ratio)
      sta::check_positive_float "-aspect_ratio" $aspect_ratio
    } else {
      set aspect_ratio 1.0
    }
    ifp::init_floorplan_util $util $aspect_ratio \
      [ord::microns_to_dbu $core_sp_bottom] \
      [ord::microns_to_dbu $core_sp_top] \
      [ord::microns_to_dbu $core_sp_left] \
      [ord::microns_to_dbu $core_sp_right] \
      $site \
      $additional_sites \
      $row_parity
  } elseif { [info exists keys(-die_area)] } {
    set die_area $keys(-die_area)
    if { [llength $die_area] != 4 } {
      utl::error IFP 15 "-die_area is a list of 4 coordinates."
    }
    lassign $die_area die_lx die_ly die_ux die_uy
    sta::check_positive_float "-die_area" $die_lx
    sta::check_positive_float "-die_area" $die_ly
    sta::check_positive_float "-die_area" $die_ux
    sta::check_positive_float "-die_area" $die_uy

    ord::ensure_linked
    if { [info exists keys(-core_area)] } {
      set core_area $keys(-core_area)
      if { [llength $core_area] != 4 } {
        utl::error IFP 16 "-core_area is a list of 4 coordinates."
      }
      lassign $core_area core_lx core_ly core_ux core_uy
      sta::check_positive_float "-core_area" $core_lx
      sta::check_positive_float "-core_area" $core_ly
      sta::check_positive_float "-core_area" $core_ux
      sta::check_positive_float "-core_area" $core_uy

      # convert die/core coordinates to dbu.
      ifp::init_floorplan_core \
        [ord::microns_to_dbu $die_lx] [ord::microns_to_dbu $die_ly] \
        [ord::microns_to_dbu $die_ux] [ord::microns_to_dbu $die_uy] \
        [ord::microns_to_dbu $core_lx] [ord::microns_to_dbu $core_ly] \
        [ord::microns_to_dbu $core_ux] [ord::microns_to_dbu $core_uy] \
        $site \
        $additional_sites \
        $row_parity
    } else {
      utl::error IFP 17 "no -core_area specified."
    }
  } else {
    utl::error IFP 19 "no -utilization or -die_area specified."
  }
}

sta::define_cmd_args "make_tracks" {[layer]\
                                      [-x_pitch x_pitch]\
                                      [-y_pitch y_pitch]\
                                      [-x_offset x_offset]\
                                      [-y_offset y_offset]}

proc make_tracks { args } {
  sta::parse_key_args "make_tracks" args \
    keys {-x_pitch -y_pitch -x_offset -y_offset} \
    flags {}

  sta::check_argc_eq0or1 "initialize_floorplan" $args

  set tech [ord::get_db_tech]

  if { [llength $args] == 0 } {
    ifp::make_layer_tracks
  } elseif { [llength $args] == 1 } {
    set layer_name [lindex $args 0]
    set layer [$tech findLayer $layer_name]
    if { $layer == "NULL" } {
      utl::error "IFP" 10 "layer $layer_name not found."
    }
    if { [$layer getType] != "ROUTING" } {
      utl::error "IFP" 25 "layer $layer_name is not a routing layer."
    }

    if { [info exists keys(-x_pitch)] } {
      set x_pitch $keys(-x_pitch)
      set x_pitch [ifp::microns_to_mfg_grid $x_pitch]
      sta::check_positive_float "-x_pitch" $x_pitch
    } else {
      set x_pitch [$layer getPitchX]
    }

    if { [info exists keys(-x_offset)] } {
      set x_offset $keys(-x_offset)
      sta::check_positive_float "-x_offset" $x_offset
      set x_offset [ifp::microns_to_mfg_grid $x_offset]
    } else {
      set x_offset [$layer getOffsetX]
    }

    if { [info exists keys(-y_pitch)] } {
      set y_pitch $keys(-y_pitch)
      set y_pitch [ifp::microns_to_mfg_grid $y_pitch]
      sta::check_positive_float "-y_pitch" $y_pitch
    } else {
      set y_pitch [$layer getPitchY]
    }

    if { [info exists keys(-y_offset)] } {
      set y_offset $keys(-y_offset)
      sta::check_positive_float "-y_offset" $y_offset
      set y_offset [ifp::microns_to_mfg_grid $y_offset]
    } else {
      set y_offset [$layer getOffsetY]
    }
    ifp::make_layer_tracks $layer $x_offset $x_pitch $y_offset $y_pitch
  }
}

sta::define_cmd_args "insert_tiecells" {tie_pin \
                                        [-prefix prefix]
}

proc insert_tiecells { args } {
  sta::parse_key_args "insert_tiecells" args \
    keys {-prefix} \
    flags {}

  sta::check_argc_eq1 "insert_tiecells" $args

  set prefix "TIEOFF_"
  if { [info exists keys(-prefix)] } {
    set prefix $keys(-prefix)
  }

  set tie_pin_split [split $args {/}]
  set port [lindex $tie_pin_split end]
  set tie_cell [join [lrange $tie_pin_split 0 end-1] {/}]

  set master NULL
  foreach lib [[ord::get_db] getLibs] {
    set master [$lib findMaster $tie_cell]
    if { $master != "NULL" } {
      break
    }
  }
  if { $master == "NULL" } {
    utl::error "IFP" 31 "Unable to find master: $tie_cell"
  }

  set mterm [$master findMTerm $port]
  if { $master == "NULL" } {
    utl::error "IFP" 32 "Unable to find master pin: $args"
  }

  ifp::insert_tiecells_cmd $mterm $prefix
}

namespace eval ifp {
proc microns_to_mfg_grid { microns } {
  set tech [ord::get_db_tech]
  if { [$tech hasManufacturingGrid] } {
    set dbu [$tech getDbUnitsPerMicron]
    set grid [$tech getManufacturingGrid]
    return [expr round(round($microns * $dbu / $grid) * $grid)]
  } else {
    return [ord::microns_to_dbu $microns]
  }
}
}
