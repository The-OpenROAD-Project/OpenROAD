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

proc show_openroad_splash {} {
  puts "OpenROAD [ord::openroad_version] [string range [ord::openroad_git_sha1] 0 9]
This program is licensed under the BSD-3 license. See the LICENSE file for details. 
Components of this program may be licensed under more restrictive licenses which must be honored."
}

# -library is the default
sta::define_cmd_args "read_lef" {[-tech] [-library] filename}

proc read_lef { args } {
  sta::parse_key_args "read_lef" args keys {} flags {-tech -library}
  sta::check_argc_eq1 "read_lef" $args

  set filename [file nativename $args]
  if { ![file exists $filename] } {
    ord::error ORD 1 "$filename does not exist."
  }
  if { ![file readable $filename] } {
    ord::error ORD 2 "$filename is not readable."
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

sta::define_cmd_args "read_def" {[-order_wires] [-continue_on_errors] filename}

proc read_def { args } {
  sta::parse_key_args "read_def" args keys {} flags {-order_wires -continue_on_errors}
  sta::check_argc_eq1 "read_def" $args
  set filename [file nativename $args]
  if { ![file exists $filename] } {
    ord::error ORD 3 "$filename does not exist."
  }
  if { ![file readable $filename] } {
    ord::error ORD 4 "$filename is not readable."
  }
  if { ![ord::db_has_tech] } {
    ord::error ORD 5 "no technology has been read."
  }
  set order_wires [info exists flags(-order_wires)]
  set continue_on_errors [info exists flags(-continue_on_errors)]
  ord::read_def_cmd $filename $order_wires $continue_on_errors
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
      ord::error ORD 6 "DEF versions 5.8, 5.6, 5.4, 5.3 supported."
    }
  }

  sta::check_argc_eq1 "write_def" $args
  set filename [file nativename $args]
  ord::write_def_cmd $filename $version
}

sta::define_cmd_args "read_db" {filename}

proc read_db { args } {
  sta::check_argc_eq1 "read_db" $args
  set filename [file nativename $args]
  if { ![file exists $filename] } {
    ord::error ORD 7 "$filename does not exist."
  }
  if { ![file readable $filename] } {
    ord::error ORD 8 "$filename is not readable."
  }
  ord::read_db_cmd $filename
}

sta::define_cmd_args "write_db" {filename}

proc write_db { args } {
  sta::check_argc_eq1 "write_db" $args
  set filename [file nativename $args]
  ord::write_db_cmd $filename
}

sta::define_cmd_args "create_physical_cluster" {cluster_name}

proc create_physical_cluster { args } {
  sta::check_argc_eq1 "create_physical_cluster" $args
  set cluster_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [odb::dbGroup_create $block $cluster_name]
  if { $group == "NULL" } {
    ord::error "duplicate group name"
  }
}

sta::define_cmd_args "create_child_physical_clusters" {[-module module_name | -modinst path]}

proc create_child_physical_clusters { args } {
  sta::parse_key_args "create_child_physical_clusters" args keys {-module -modinst} flags {}
  set cluster_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  if { [info exists keys(-module)] } {
    set module [$block findModule $keys(-module)]
  }
  if { [info exists keys(-modinst)] } {
    set module [[$block findModInst $keys(-modinst)] getMaster]
  }
  if { $module == "NULL" } {
    ord::error "module does not exist"
  }
  set module_instance [$module getModInst]
  set modinsts [$module getChildren]
  set insts [$module getInsts]
  foreach modinst $modinsts {
    set cluster_name "[$module getName]_[$modinst getName]"
    set group [odb::dbGroup_create $block $cluster_name]
    if { $group == "NULL" } {
      ord::error "duplicate group name"
    }
    $group addModInst $modinst
  }
  if { [llength $insts] > 0 } {
    if { $module_instance == "NULL" } {
      set group [odb::dbGroup_create $block "[$module getName]_glue"]
    } else {
      set parent [$module_instance getParent]
      set group [odb::dbGroup_create $block "[$parent getName]_[$module_instance getName]_glue"]
    }
    foreach inst $insts {
      $group addInst $inst
    }
  }
}

sta::define_cmd_args "create_voltage_domain" {domain_name llx lly urx ury}

proc create_voltage_domain { args } {
  if { [llength $args] != 5 } {
    sta::sta_error "create_voltage_domain requires five positional arguments."
  }
  set domain_name [lindex $args 0]
  set llx [lindex $args 1]
  set lly [lindex $args 2]
  set urx [lindex $args 3]
  set ury [lindex $args 4]
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [odb::dbGroup_create $block $domain_name $llx $lly $urx $ury]
  if { $group == "NULL" } {
    ord::error "duplicate group name"
  }
}

sta::define_cmd_args "delete_physical_cluster" {cluster_name}

proc delete_physical_cluster { args } {
  sta::check_argc_eq1 "delete_physical_cluster" $args
  set cluster_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [$block findGroup $cluster_name]
  if { $group == "NULL" } {
    ord::error "group does not exist"
  }
  if { [$group getType] == 1 } {
    ord::error "group is not of physical cluster type"
  }
  odb::dbGroup_destroy $group
}

sta::define_cmd_args "delete_voltage_domain" {domain_name}

proc delete_voltage_domain { args } {
  sta::check_argc_eq1 "delete_voltage_domain" $args
  set domain_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [$block findGroup $domain_name]
  if { $group == "NULL" } {
    ord::error "group does not exist"
  }
  if { [$group getType] == 0 } {
    ord::error "group is not of voltage domain type"
  }
  odb::dbGroup_destroy $group
}

sta::define_cmd_args "assign_power_net" {domain_name net_name}

proc assign_power_net { args } {
  sta::check_argc_eq2 "assign_power_net" $args
  set domain_name [lindex $args 0]
  set net_name [lindex $args 1]
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [$block findGroup $domain_name]
  set net [$block findNet $net_name]
  if { $group == "NULL" } {
    ord::error "group does not exist"
  }
  if { [$group getType] == 0 } {
    ord::error "group is not of voltage domain type"
  }
  if { $net == "NULL" } {
    ord::error "net does not exist"
  }
  $group addPowerNet $net
}

sta::define_cmd_args "assign_ground_net" {domain_name net_name}

proc assign_ground_net { args } {
  sta::check_argc_eq2 "assign_ground_net" $args
  set domain_name [lindex $args 0]
  set net_name [lindex $args 1]
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [$block findGroup $domain_name]
  set net [$block findNet $net_name]
  if { $group == "NULL" } {
    ord::error "group does not exist"
  }
  if { [$group getType] == 0 } {
    ord::error "group is not of voltage domain type"
  }
  if { $net == "NULL" } {
    ord::error "net does not exist"
  }
  $group addGroundNet $net
}

sta::define_cmd_args "add_to_physical_cluster" { [-parent_module module_name -modinst modinst_name | -inst inst_name | -physical_cluster cluster_name]  cluster_name }

proc add_to_physical_cluster { args } {
  sta::parse_key_args "add_to_physical_cluster" args keys {-parent_module -modinst -inst -physical_cluster} flags {}
  sta::check_argc_eq1 "add_to_physical_cluster" $args
  set cluster_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [$block findGroup $cluster_name]
  if { $group == "NULL" } {
    ord::error "cluster does not exist"
  }
  if { [$group getType] == 1 } {
    ord::error "group is not of physical cluster type"
  }
  if { [info exists keys(-parent_module)] } {
    set module [$block findModule $keys(-parent_module)]
    if { $module == "NULL" } {
      ord::error "parent module does not exist"
    }
    set modinst [$module findModInst $keys(-modinst)]
    if { $modinst == "NULL" } {
      ord::error "modinst does not exist"
    }
    $group addModInst $modinst
  }
  if { [info exists keys(-inst)] } {
    set inst [$block findInst $keys(-inst)]
    if { $inst == "NULL" } {
      ord::error "inst does not exist"
    }
    $group addInst $inst
  }
  if { [info exists keys(-physical_cluster)] } {
    set child [$block findGroup $keys(-physical_cluster)]
    if { $child == "NULL" } {
      ord::error "child physical cluster does not exist"
    }
    if { [$child getType] == 1 } {
      ord::error "child group is not of physical cluster type"
    }
    $group addGroup $child
  }
}

sta::define_cmd_args "remove_from_physical_cluster" { [-parent_module module_name -modinst modinst_name | -inst inst_name | -physical_cluster cluster_name]  cluster_name }

proc remove_from_physical_cluster { args } {
  sta::parse_key_args "remove_from_physical_cluster" args keys {-parent_module -modinst -inst -physical_cluster} flags {}
  sta::check_argc_eq1 "remove_from_physical_cluster" $args
  set cluster_name $args
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set group [$block findGroup $cluster_name]
  if { $group == "NULL" } {
    ord::error "cluster does not exist"
  }
  if { [$group getType] == 1 } {
    ord::error "group is not of physical cluster type"
  }
  if { [info exists keys(-parent_module)] } {
    set module [$block findModule $keys(-parent_module)]
    if { $module == "NULL" } {
      ord::error "parent module does not exist"
    }
    set modinst [$module findModInst $keys(-modinst)]
    if { $modinst == "NULL" } {
      ord::error "modinst does not exist"
    }
    $group removeModInst $modinst
  }
  if { [info exists keys(-inst)] } {
    set inst [$block findInst $keys(-inst)]
    if { $inst == "NULL" } {
      ord::error "inst does not exist"
    }
    $group removeInst $inst
  }
  if { [info exists keys(-physical_cluster)] } {
    set child [$block findGroup $keys(-physical_cluster)]
    if { $child == "NULL" } {
      ord::error "child physical cluster does not exist"
    }
    if { [$child getType] == 1 } {
      ord::error "child group is not of physical cluster type"
    }
    $group removeGroup $child
  }
}

sta::define_cmd_args "report_physical_clusters" {}

proc report_physical_clusters {} {
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set groups [$block getGroups]
  puts "\nReporting Physical Clusters"
  foreach group $groups {
    if { [$group getType] == 0 } {
      report_group $group
    }
  }
  puts ""
}

sta::define_cmd_args "report_voltage_domains" {}

proc report_voltage_domains {} {
  set db [ord::get_db]
  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error "please load the design before trying to use this command"
  }
  set block [$chip getBlock]
  set groups [$block getGroups]
  puts "\nReporting Voltage Domains"
  foreach group $groups {
    if { [$group getType] == 1 } {
      report_group $group
    }
  }
  puts ""
}

proc report_group { group } {
  if { [$group getType] == 0 } {
    puts -nonewline "\nPhysical Cluster : "
  } else {
    puts -nonewline "\nVoltage Domain : "
  }
  puts -nonewline "[$group getName]"
  if { [$group hasBox] } {
    set rect [$group getBox]
    puts -nonewline "\nBox : ([$rect xMin],[$rect yMin]) ([$rect xMax],[$rect yMax])"
  }
  set modinsts [$group getModInsts]
  set insts [$group getInsts]
  set children [$group getGroups]
  set powerNets [$group getPowerNets]
  set groundNets [$group getGroundNets]
  if { [llength $modinsts] > 0 } {
    puts -nonewline "\nModInsts: "
    foreach modinst $modinsts {
      puts -nonewline "[$modinst getName] "
    }
  }

  if { [llength $insts] > 0 } {
    puts -nonewline "\nInsts: "
    foreach inst $insts {
      puts -nonewline "[$inst getName] "
    }
  }

  if { [llength $children] > 0 } {
    puts -nonewline "\nChildren: "
    foreach child $children {
      puts -nonewline "[$child getName] "
    }
  }

  if { [llength $powerNets] > 0 } {
    puts -nonewline "\nPower Nets: "
    foreach net $powerNets {
      puts -nonewline "[$net getName] "
    }
  }
  
  if { [llength $groundNets] > 0 } {
    puts -nonewline "\nGround Nets: "
    foreach net $groundNets {
      puts -nonewline "[$net getName] "
    }
  }
  puts ""
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

  if { ![info exists keys(-layer)] && ![info exists keys(-via)] } {
    ord::error ORD 9 "layer or via must be specified."
  }

  if { [info exists keys(-layer)] && [info exists keys(-via)] } {
    ord::error ORD 10 "Exactly one of layer or via must be specified."
  }

  set db [ord::get_db]
  set tech [$db getTech]

  if { [info exists keys(-layer)] } {
    set techLayer [$tech findLayer $keys(-layer)]
  } else {
    set techLayer [$tech findLayer $keys(-via)]
  }

  set chip [$db getChip]
  if { $chip == "NULL" } {
    ord::error ORD 11 "please load the design before trying to use this command"
  }
  set block [$chip getBlock]

  if { $techLayer == "NULL" } {
    ord::error "layer not found."
  }

  if { ![info exists keys(-capacitance)] && ![info exists keys(-resistance)] } {
    ord::error ORD 12 "use -capacitance <value> or -resistance <value>."
  }

  if { [info exists keys(-via)] } {
    set viaTechLayer [$techLayer getUpperLayer]

    if { [info exists keys(-capacitance)] } {
      set wire_cap [capacitance_ui_sta $keys(-capacitance)]
      $viaTechLayer setCapacitance $wire_cap
    }

    if { [info exists keys(-resistance)] } {
      set wire_res [resistance_ui_sta $keys(-resistance)]
      $viaTechLayer setResistance $wire_res
    }

    return
  }

  if { [info exists keys(-capacitance)] } {
    # Zero the edge cap and just use the user given value
    $techLayer setEdgeCapacitance 0
    # The DB stores capacitance per square micron of area, not per
    # micron of length. "1.0" is just for casting integer to float
    set dbu [expr 1.0 * [$block getDbUnitsPerMicron]]
    set wire_width [expr 1.0 * [$techLayer getWidth] / $dbu]
    set wire_cap [expr [sta::capacitance_ui_sta $keys(-capacitance)] / [sta::distance_ui_sta 1.0]]
    # ui_sta converts to F/m so multiple by 1E6 to get pF/um
    set cap_per_square [expr 1E6 * $wire_cap / $wire_width]

    $techLayer setCapacitance $cap_per_square
  }

  if { [info exists keys(-resistance)] } {
    # The DB stores resistance for a square of wire,
    # not unit resistance. "1.0" is just for casting integer to float
    set wire_width [expr 1.0 * [$techLayer getWidth] / [$block getDbUnitsPerMicron]]
    set wire_res [expr [sta::resistance_ui_sta $keys(-resistance)] / [sta::distance_ui_sta 1.0]]
    # ui_sta converts to ohm/m so multiple by 1E-6 to get ohm/um
    set res_per_square [expr 1e-6 * $wire_width * $wire_res]

    $techLayer setResistance $res_per_square
  }
}

################################################################

namespace eval ord {

trace variable ::file_continue_on_error "w" \
  ord::trace_file_continue_on_error

# Sync with sta::sta_continue_on_error used by 'source' proc defined by OpenSTA.
proc trace_file_continue_on_error { name1 name2 op } {
  set ::sta_continue_on_error $::file_continue_on_error
}

proc error { args } {
  if { [llength $args] == 1 } {
    # pre-logger compatibility
    ord_error UKN 0 [lindex $args 0]
  } elseif { [llength $args] == 3 } {
    lassign $args tool_id id msg
    ord_error $tool_id $id $msg
  } else {
    ord_error UKN 0 "ill-formed error arguments $args"
  }
}

proc warn { args } {
  if { [llength $args] == 1 } {
    # pre-logger compatibility
    ord_warn UKN 0 [lindex $args 0]
  } elseif { [llength $args] == 3 } {
    lassign $args tool_id id msg
    ord_warn $tool_id $id $msg
  } else {
    ord_warn UKN 0 "ill-formed warn arguments $args"
  }
}

proc ensure_units_initialized { } {
  if { ![units_initialized] } {
    ord::error ORD 13 "command units uninitialized. Use the read_liberty or set_cmd_units command to set units."
  }
}

proc clear {} {
  sta::clear_network
  sta::clear_sta
  grt::clear_fastroute
  [get_db] clear
}

# namespace ord
}

# redefine sta::sta_error to call ord::error
namespace eval sta {

proc sta_error { msg } {
  ord::error STA 0 $msg
}

proc sta_warn { msg } {
  ord::warn STA 0 $msg
}

# namespace sta
}
