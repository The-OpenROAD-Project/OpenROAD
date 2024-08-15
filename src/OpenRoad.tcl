############################################################################
##
## Copyright (c) 2019, The Regents of the University of California
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
sta::define_cmd_args "read_lef" {[-tech] [-library] [-tech_name name] filename}

proc read_lef { args } {
  sta::parse_key_args "read_lef" args keys {-tech_name} flags {-tech -library}
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
    if { [info exists keys(-tech_name)] } {
      set make_tech 1
    } else {
      set make_tech [expr ![ord::db_has_tech]]
    }
  }

  set tech_name ""
  set lib_name [file rootname [file tail $filename]]
  if { [info exists keys(-tech_name)] } {
    set tech_name $keys(-tech_name)
  } elseif { $make_tech } {
    set tech_name $lib_name
  }

  ord::read_lef_cmd $filename $lib_name $tech_name $make_tech $make_lib
}

sta::define_cmd_args "read_def" {[-floorplan_initialize|-incremental|-child]\
                                   [-continue_on_errors]\
                                   [-tech name] \
                                   filename}

proc read_def { args } {
  sta::parse_key_args "read_def" args keys {-tech} \
    flags {-floorplan_initialize -incremental \
           -order_wires -continue_on_errors -child}
  sta::check_argc_eq1 "read_def" $args
  set filename [file nativename [lindex $args 0]]
  if { ![file exists $filename] } {
    utl::error "ORD" 3 "$filename does not exist."
  }
  if { ![file readable $filename] || ![file isfile $filename] } {
    utl::error "ORD" 4 "$filename is not readable."
  }
  set tech_name ""
  if { [info exists keys(-tech)] } {
    set tech_name $keys(-tech)
  } elseif { ![ord::db_has_tech] } {
    utl::error "ORD" 5 "No technology has been read."
  }
  if { [info exists flags(-order_wires)] } {
    utl::warn "ORD" 33 "-order_wires is deprecated."
  }
  set continue_on_errors [info exists flags(-continue_on_errors)]
  set floorplan_init [info exists flags(-floorplan_initialize)]
  set incremental [info exists flags(-incremental)]
  set child [info exists flags(-child)]
  if { $floorplan_init + $incremental + $child > 1} {
    utl::error ORD 16 "Options -incremental, -floorplan_initialization,\
      and -child are mutually exclusive."
  }
  ord::read_def_cmd $filename $tech_name $continue_on_errors $floorplan_init \
    $incremental $child
}

sta::define_cmd_args "write_def" {[-version version] filename}

proc write_def { args } {
  sta::parse_key_args "write_def" args keys {-version} flags {}

  set version "5.8"
  if { [info exists keys(-version)] } {
    set version $keys(-version)
    if { !($version == "5.8" \
        || $version == "5.7" \
        || $version == "5.6" \
        || $version == "5.5" \
        || $version == "5.4" \
        || $version == "5.3") } {
      utl::error "ORD" 6 "DEF versions 5.8, 5.7, 5.6, 5.5, 5.4, 5.3 supported."
    }
  }

  sta::check_argc_eq1 "write_def" $args
  set filename [file nativename [lindex $args 0]]
  ord::write_def_cmd $filename $version
}

sta::define_cmd_args "write_abstract_lef" {[-bloat_factor amount|-bloat_occupied_layers] filename}
sta::define_cmd_args "write_lef" {filename}

proc write_lef { args } {
  sta::parse_key_args "write_lef" args keys {} flags {}

  sta::check_argc_eq1 "write_lef" $args
  set filename [file nativename [lindex $args 0]]
  ord::write_lef_cmd $filename
}

proc write_abstract_lef { args } {
  sta::parse_key_args "write_abstract_lef" args keys {-bloat_factor} flags {-bloat_occupied_layers}

  set bloat_factor 10
  if { [info exists keys(-bloat_factor)] } {
    set bloat_factor $keys(-bloat_factor)
    sta::check_positive_float "bloat_factor" $bloat_factor
  }

  set bloat_occupied_layers [info exists flags(-bloat_occupied_layers)]
  if { [info exists keys(-bloat_factor)] && $bloat_occupied_layers } {
    utl::error ORD 1050 "Options -bloat and -bloat_occupied_layers are\
      both set. At most one should be used."
  }

  sta::check_argc_eq1 "write_abstract_lef" $args
  set filename [file nativename [lindex $args 0]]
  ord::write_abstract_lef_cmd $filename $bloat_factor $bloat_occupied_layers
}

sta::define_cmd_args "write_cdl" {[-include_fillers]
    -masters masters_filenames out_filename }

proc write_cdl { args } {

  sta::parse_key_args "write_cdl" args keys {-masters} flags {-include_fillers}
  set fillers [info exists flags(-include_fillers)]
  sta::check_argc_eq1 "write_cdl" $args
  if {![info exists keys(-masters)]} {
    utl::error ORD 1013 "-masters is required."
  }
  set out_filename [file nativename [lindex $args 0]]
  set masters_filenames []
  foreach masters_filename $keys(-masters) {
    lappend masters_filenames [file nativename $masters_filename]
  }
  ord::write_cdl_cmd $out_filename $masters_filenames $fillers
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

sta::define_cmd_args "assign_ndr" { -ndr name (-net name | -all_clocks) }

proc assign_ndr { args } {
  sta::parse_key_args "assign_ndr" args keys {-ndr -net} flags {-all_clocks}
  if { ![info exists keys(-ndr)] } {
    utl::error ORD 1009 "-name is missing."
  }
  if { !([info exists keys(-net)] ^ [info exists flags(-all_clocks)]) } {
    utl::error ORD 1010 "Either -net or -all_clocks need to be defined."
  }
  set block [[[ord::get_db] getChip] getBlock]
  set ndrName $keys(-ndr)
  set ndr [$block findNonDefaultRule $ndrName]
  if { $ndr == "NULL" } {
    utl::error ORD 1011 "No NDR named ${ndrName} found."
  }
  if { [info exists keys(-net)] } {
    set netName $keys(-net)
    set net [$block findNet $netName]
    if { $net == "NULL" } {
      utl::error ORD 1012 "No net named ${netName} found."
    }
    $net setNonDefaultRule $ndr
  } else {
    foreach net [sta::find_all_clk_nets] {
      $net setNonDefaultRule $ndr
    }
  }
}

sta::define_cmd_args "set_debug_level" { tool group level }
proc set_debug_level {args} {
  sta::check_argc_eq3 "set_debug_level" $args
  lassign $args tool group level
  sta::check_integer "set_debug_level" $level
  ord::set_debug_level $tool $group $level
}

sta::define_cmd_args "suppress_message" { tool id }
proc suppress_message {args} {
  sta::check_argc_eq2 "suppress_message" $args
  lassign $args tool id
  sta::check_integer "suppress_message_level" $id
  utl::suppress_message $tool $id
}

sta::define_cmd_args "unsuppress_message" { tool id }
proc unsuppress_message {args} {
  sta::check_argc_eq2 "unsuppress_message" $args
  lassign $args tool id
  sta::check_integer "unsuppress_message_level" $id
  utl::unsuppress_message $tool $id
}

proc set_thread_count { count } {
  ord::set_thread_count $count
}

proc thread_count { } {
  return [ord::thread_count]
}

sta::define_cmd_args "global_connect" {}
proc global_connect {} {
  [ord::get_db_block] globalConnect
}

sta::define_cmd_args "clear_global_connect" {}
proc clear_global_connect {} {
  [ord::get_db_block] clearGlobalConnect
}

sta::define_cmd_args "report_global_connect" {}
proc report_global_connect {} {
  [ord::get_db_block] reportGlobalConnect
}

sta::define_cmd_args "add_global_connection" {[-net net_name] \
                                              [-inst_pattern inst_name_pattern] \
                                              [-pin_pattern pin_name_pattern] \
                                              [(-power|-ground)] \
                                              [-region region_name] \
                                              [-defer_connection]
}
proc add_global_connection {args} {
  sta::parse_key_args "add_global_connection" args \
    keys {-net -inst_pattern -pin_pattern -region} \
    flags {-power -ground -defer_connection}

  sta::check_argc_eq0 "add_global_connection" $args

  if {[info exists flags(-power)] && [info exists flags(-ground)]} {
    utl::error ORD 41 "The flags -power and -ground of the\
      add_global_connection command are mutually exclusive."
  }

  if {![info exists keys(-net)]} {
    utl::error ORD 42 "The -net option of the add_global_connection command is required."
  }

  if {![info exists keys(-inst_pattern)]} {
    set keys(-inst_pattern) {.*}
  }

  if {![info exists keys(-pin_pattern)]} {
    utl::error ORD 43 "The -pin_pattern option of the add_global_connection command is required."
  }

  set net [[ord::get_db_block] findNet $keys(-net)]
  if {$net == "NULL"} {
    set net [odb::dbNet_create [ord::get_db_block] $keys(-net)]
    if {![info exists flags(-power)] && ![info exists flags(-ground)]} {
      utl::warn ORD 44 "Net created for $keys(-net), if intended as power\
        or ground net add the -power/-ground switch as appropriate."
    }
  }

  if {[info exists flags(-power)]} {
    $net setSpecial
    $net setSigType POWER
  } elseif {[info exists flags(-ground)]} {
    $net setSpecial
    $net setSigType GROUND
  }

  set do_connect 1
  if {[info exists flags(-defer_connection)]} {
    utl::warn ORD 46 "-defer_connection has been deprecated."
    set do_connect 0
  }

  set region "NULL"
  if {[info exists keys(-region)]} {
    set region [[ord::get_db_block] findRegion $keys(-region)]
    if {$region == "NULL"} {
      utl::error ORD 45 "Region \"$keys(-region)\" not defined"
    }
  }

  [ord::get_db_block] addGlobalConnect $region $keys(-inst_pattern) \
    $keys(-pin_pattern) $net $do_connect
}

################################################################

namespace eval ord {

proc ensure_units_initialized { } {
  if { ![units_initialized] } {
    utl::error "ORD" 13 "Command units uninitialized. Use the\
      read_liberty or set_cmd_units command to set units."
  }
}

proc clear {} {
  sta::clear_network
  sta::clear_sta
  grt::clear
  [get_db] clear
}

proc profile_cmd {filename args} {
  utl::info 99 "Profiling $args > $filename."
  profile -commands on
  if {[catch "{*}$args"]} { ;# tclint-disable-line command-args
    global errorInfo
    puts $errorInfo
  }
  profile off profarray
  profrep profarray cpu $filename
}

proc get_die_area { } {
  set area {}
  set rect [[ord::get_db_block] getDieArea]
  lappend area [ord::dbu_to_microns [$rect xMin]]
  lappend area [ord::dbu_to_microns [$rect yMin]]
  lappend area [ord::dbu_to_microns [$rect xMax]]
  lappend area [ord::dbu_to_microns [$rect yMax]]
  return $area
}

proc get_core_area { } {
  set area {}
  set rect [[ord::get_db_block] getCoreArea]
  lappend area [ord::dbu_to_microns [$rect xMin]]
  lappend area [ord::dbu_to_microns [$rect yMin]]
  lappend area [ord::dbu_to_microns [$rect xMax]]
  lappend area [ord::dbu_to_microns [$rect yMax]]
  return $area
}

proc parse_list_args {cmd arg_var list_var lists_args} {
  upvar 1 $arg_var args
  upvar 1 $list_var list

  foreach arg_opt $lists_args {
    set remaining_args []

    set list($arg_opt) []
    for {set i 0} {$i < [llength $args]} {incr i} {
      set arg [lindex $args $i]
      if { [sta::is_keyword_arg $arg] } {
        if { $arg == $arg_opt } {
          incr i
          if { [llength $args] == $i } {
            utl::error ORD 560 "$cmd $arg_opt missing value."
          }
          lappend list($arg_opt) [lindex $args $i]
        } else {
          lappend remaining_args $arg
        }
      } else {
        lappend remaining_args $arg
      }
    }

    set args $remaining_args
  }
}

# namespace ord
}
