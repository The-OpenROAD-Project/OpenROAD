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

sta::define_cmd_args "read_def" {[-floorplan_initialize|-incremental]\
                                   [-continue_on_errors]\
                                   filename}

proc read_def { args } {
  sta::parse_key_args "read_def" args keys {} flags {-floorplan_initialize -incremental\
                                                       -order_wires -continue_on_errors}
  sta::check_argc_eq1 "read_def" $args
  set filename [file nativename [lindex $args 0]]
  if { ![file exists $filename] } {
    utl::error "ORD" 3 "$filename does not exist."
  }
  if { ![file readable $filename] || ![file isfile $filename] } {
    utl::error "ORD" 4 "$filename is not readable."
  }
  if { ![ord::db_has_tech] } {
    utl::error "ORD" 5 "no technology has been read."
  }
  if { [info exists flags(-order_wires)] } {
    utl::warn "ORD" 33 "-order_wires is deprecated."
  }
  set continue_on_errors [info exists flags(-continue_on_errors)]
  set floorplan_init [info exists flags(-floorplan_initialize)]
  set incremental [info exists flags(-incremental)]
  if { $floorplan_init && $incremental } {
    utl::error ORD 16 "incremental and floorplan_initialization options are both set. At most one should be used."
  }
  ord::read_def_cmd $filename $continue_on_errors $floorplan_init $incremental
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

# partition
sta::define_cmd_args "partition_design" { [-max_num_macro max_num_macro] \
                                          [-min_num_macro min_num_macro] \
                                          [-max_num_inst max_num_inst] \
                                          [-min_num_inst min_num_inst] \
                                          [-net_threshold net_threshold] \
                                          [-virtual_weight virtual_weight] \
                                          [-ignore_net_threshold ignore_net_threshold] \
                                          [-rpt_file rpt_file] \
                                        }
proc partition_design { args } {
    sta::parse_key_args "partition_design" args keys {-max_num_macro -min_num_macro
                     -max_num_inst  -min_num_inst -net_threshold -virtual_weight -ignore_net_threshold -rpt_file} flags {  }
    if { [info exists keys(-rpt_file)] } {
        set rpt_file $keys(-rpt_file)
        set max_num_macro 10
        set min_num_macro 2
        set max_num_inst 0
        set min_num_inst 0
        set net_threshold 0
        set virtual_weight 50
        set ignore_net_threshold 0
 
        if { [info exists keys(-max_num_macro)] } {
            set max_num_macro $keys(-max_num_macro)
        }
 
        if { [info exists keys(-min_num_macro)] } {
            set min_num_macro $keys(-min_num_macro)
        }

        if { [info exists keys(-max_num_inst)] } {
            set max_num_inst $keys(-max_num_inst)
        }
 
        if { [info exists keys(-min_num_inst)] } {
            set min_num_inst $keys(-min_num_inst)
        }
 
        if { [info exists keys(-net_threshold)] } {
            set net_threshold $keys(-net_threshold)
        }

        if { [info exists keys(-virtual_weight)] } {
            set virtual_weight $keys(-virtual_weight)
        }

        if { [info exists keys(-ignore_net_threshold)] } {
            set net_threshold $keys(-ignore_net_threshold)
        }

        ord::partition_design_cmd $max_num_macro $min_num_macro $max_num_inst $min_num_inst $net_threshold $virtual_weight $ignore_net_threshold  $rpt_file
    } else {
        ord::error "partition_design -max_num_macro -min_num_macro -max_num_inst -min_num_inst -net_threshold -virtual_weight -ignore_net_threshold -rpt_file"
    }
}

# rtl_mp
sta::define_cmd_args "rtl_mp" { [-config_file config_file] \
                              }
proc rtl_mp { args } {
    sta::parse_key_args "rtl_mp" args keys { -config_file } flag {  }
    if { [info exists keys(-config_file)] } {
        set config_file $keys(-config_file)
        return [ord::rtl_mp_cmd $config_file]
    } else {
        ord::error "rtl_mp  -config_file config_file"
    }
    
    set block [ord::get_db_block]
    set units [$block getDefUnits]
    set macro_placement_file "./rtl_mp/macro_placement.cfg"

    set ch [open $macro_placement_file]

    while {![eof $ch]} {
        set line [gets $ch]
        if {[llength $line] == 0} {continue}

        set inst_name [lindex $line 0]
        set orientation [lindex $line 1]
        set x [expr round([lindex $line 2] * $units)]
        set y [expr round([lindex $line 3] * $units)]

        if {[set inst [$block findInst $inst_name]] == "NULL"} {
            error "Cannot find instance $inst_name"
        }

        $inst setOrient $orientation
        $inst setOrigin $x $y
        $inst setPlacementStatus FIRM
    }

    close $ch
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

sta::define_cmd_args "assign_ndr" { -ndr name (-net name | -all_clocks) }

proc assign_ndr { args } {
  sta::parse_key_args "assign_ndr" args keys {-ndr -net} flags {-all_clocks}
  if { ![info exists keys(-ndr)] } {
    utl::error ORD 1009 "-name is missing"
  }
  if { ! ([info exists keys(-net)] ^ [info exists flags(-all_clocks)]) } {
    utl::error ORD 1010 "Either -net or -all_clocks need to be defined"
  }
  set block [[[ord::get_db] getChip] getBlock]
  set ndrName $keys(-ndr)
  set ndr [$block findNonDefaultRule $ndrName]
  if { $ndr == "NULL" } {
    utl::error ORD 1011 "No NDR named ${ndrName} found"
  }
  if { [info exists keys(-net)] } {
    set netName $keys(-net)
    set net [$block findNet $netName]
    if { $net == "NULL" } {
      utl::error ORD 1012 "No net named ${netName} found"
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

sta::define_cmd_args "python" { args }
proc python {args} {
  ord::python_cmd $args
}

proc set_thread_count { count } {
  ord::set_thread_count $count
}

proc thread_count { } {
  return [ord::thread_count]
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
  
    # namespace ord
}
