############################################################################
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
##
############################################################################

# -library is the default
sta::define_cmd_args "read_lef" {[-tech] [-library] filename}

proc read_lef { args } {
  sta::parse_key_args "read_lef" args keys {} flags {-tech -library}
  sta::check_argc_eq1 "read_lef" $args

  set filename [file nativename [lindex $args 0]]
  if { ![file exists $filename] } {
    utl::error "ORD" 1 "$filename does not exist."
  }
  if { ![file readable $filename] } {
    utl::error "ORD" 2 "$filename is not readable."
  }

  set make_tech [info exists flags(-tech)]
  set make_lib [info exists flags(-library)]
  if { !$make_tech && !$make_lib} {
    set make_lib 1
    set make_tech [expr ![ord::db_has_tech]]
  }
  set lib_name [file rootname [file tail $filename]]
  ord::read_lef_cmd $filename $lib_name $make_tech $make_lib
}

sta::define_cmd_args "read_def" {[-floorplan_initialize|-incremental] [-order_wires] [-continue_on_errors] filename}

proc read_def { args } {
  sta::parse_key_args "read_def" args keys {} flags {-floorplan_initialize -incremental -order_wires -continue_on_errors}
  sta::check_argc_eq1 "read_def" $args
  set filename [file nativename [lindex $args 0]]
  if { ![file exists $filename] } {
    utl::error "ORD" 3 "$filename does not exist."
  }
  if { ![file readable $filename] } {
    utl::error "ORD" 4 "$filename is not readable."
  }
  if { ![ord::db_has_tech] } {
    utl::error "ORD" 5 "no technology has been read."
  }
  set order_wires [info exists flags(-order_wires)]
  set continue_on_errors [info exists flags(-continue_on_errors)]
  set floorplan_init [info exists flags(-floorplan_initialize)]
  set incremental [info exists flags(-incremental)]
  if { $floorplan_init && $incremental } {
    utl::error ORD 16 "incremental and floorplan_initialization options are both set. At most one should be used."
  }
  ord::read_def_cmd $filename $order_wires $continue_on_errors $floorplan_init $incremental
}

sta::define_cmd_args "write_def" {[-version version] filename}

proc write_def { args } {
  sta::parse_key_args "write_def" args keys {-version} flags {}

  set version "5.8"
  if { [info exists keys(-version)] } {
    set version $keys(-version)
    if { !($version == "5.8" \
	     || $version == "5.6" \
	     || $version == "5.5" \
	     || $version == "5.4" \
	     || $version == "5.3") } {
      utl::error "ORD" 6 "DEF versions 5.8, 5.6, 5.4, 5.3 supported."
    }
  }

  sta::check_argc_eq1 "write_def" $args
  set filename [file nativename [lindex $args 0]]
  ord::write_def_cmd $filename $version
}


sta::define_cmd_args "write_cdl" {[-include_fillers] filename}

proc write_cdl { args } {

  sta::parse_key_args "write_cdl" args keys {} flags {-include_fillers}
  set fillers [info exists flags(-include_fillers)]
  sta::check_argc_eq1 "write_cdl" $args
  set filename [file nativename [lindex $args 0]]
  ord::write_cdl_cmd $filename $fillers
}


sta::define_cmd_args "read_db" {filename}

proc read_db { args } {
  sta::check_argc_eq1 "read_db" $args
  set filename [file nativename [lindex $args 0]]
  if { ![file exists $filename] } {
    utl::error "ORD" 7 "$filename does not exist."
  }
  if { ![file readable $filename] } {
    utl::error "ORD" 8 "$filename is not readable."
  }
  ord::read_db_cmd $filename
}

sta::define_cmd_args "write_db" {filename}

proc write_db { args } {
  sta::check_argc_eq1 "write_db" $args
  set filename [file nativename [lindex $args 0]]
  ord::write_db_cmd $filename
}

# Units are from OpenSTA (ie Liberty file or set_cmd_units).
sta::define_cmd_args "set_layer_rc" { [-layer layer] \
					[-via via_layer] \
					[-capacitance cap] \
					[-resistance res] }
proc set_layer_rc {args} {
  sta::parse_key_args "set_layer_rc" args \
    keys {-layer -via -capacitance -resistance}\
    flags {}

  if { [info exists keys(-layer)] && [info exists keys(-via)] } {
    utl::error "ORD" 10 "Use -layer or -via but not both."
  }

  set tech [ord::get_db_tech]
  if { [info exists keys(-layer)] } {
    set layer_name $keys(-layer)
    set layer [$tech findLayer $layer_name]
    if { $layer == "NULL" } {
      utl::error "ORD" 19 "layer $layer_name not found."
    }

    if { $layer == "NULL" } {
      utl::error "ORD" 20 "layer not found."
    }

    if { [$layer getRoutingLevel] == 0 } {
      utl::error "ORD" 18 "$layer_name is not a routing layer."
    }

    if { [info exists keys(-capacitance)] } {
      # Zero the edge cap and just use the user given value
      $layer setEdgeCapacitance 0
      # Convert wire capacitance/wire_length to capacitance/area.
      set wire_width [ord::dbu_to_microns [$layer getWidth]]
      # F/m
      set wire_cap [expr [sta::capacitance_ui_sta $keys(-capacitance)] \
                      / [sta::distance_ui_sta 1.0]]
      # Convert to pF/um.
      set cap_per_square [expr $wire_cap * 1e+6 / $wire_width]
      $layer setCapacitance $cap_per_square
    }
    
    if { [info exists keys(-resistance)] } {
      # Convert resistance/wire_length to resistance/square.
      set wire_width [ord::dbu_to_microns [$layer getWidth]]
      # ohm/m
      set wire_res [expr [sta::resistance_ui_sta $keys(-resistance)] \
                      / [sta::distance_ui_sta 1.0]]
      # convert to ohms/square
      set res_per_square [expr $wire_width * 1e-6 * $wire_res]
      $layer setResistance $res_per_square
    }
    
    if { ![info exists keys(-capacitance)] && ![info exists keys(-resistance)] } {
      utl::error "ORD" 12 "missing -capacitance or -resistance argument."
    }
  } elseif { [info exists keys(-via)] } {
    set layer_name $keys(-via)
    set layer [$tech findLayer $layer_name]
    if { $layer == "NULL" } {
      utl::error "ORD" 21 "via $layer_name not found."
    }

    if { [info exists keys(-capacitance)] } {
      utl::warn "ORD" 22 "-capacitance not supported for vias."
    }

    if { [info exists keys(-resistance)] } {
      set via_res [sta::resistance_ui_sta $keys(-resistance)]
      $layer setResistance $via_res
    } else {
      utl::error "ORD" 17 "no -resistance specified for via."
    }
  } else {
    utl::error "ORD" 9 "missing -layer or -via argument."
  }
}

sta::define_cmd_args "set_debug_level" { tool group level }
proc set_debug_level {args} {
  sta::check_argc_eq3 "set_debug_level" $args
  lassign $args tool group level
  sta::check_integer "set_debug_level" $level
  ord::set_debug_level $tool $group $level
}

sta::define_cmd_args "python" { args }
proc python {args} {
  ord::python_cmd $args
}

################################################################

namespace eval ord {
  
  proc ensure_units_initialized { } {
    if { ![units_initialized] } {
      utl::error "ORD" 13 "command units uninitialized. Use the read_liberty or set_cmd_units command to set units."
    }
  }
  
  proc clear {} {
    sta::clear_network
    sta::clear_sta
    grt::clear_fastroute
    [get_db] clear
  }
  
  proc profile_cmd {filename args} {
    utl::info 99 "Profiling $args > $filename"
    profile -commands on
    if {[catch "{*}$args"]} {
      global errorInfo
      puts $errorInfo
    }
    profile off profarray
    profrep profarray cpu $filename
  }
  
  # namespace ord
}
