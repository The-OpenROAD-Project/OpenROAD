#BSD 3-Clause License
#
#Copyright (c) 2022, The Regents of the University of California
#All rights reserved.
#
#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are met:
#
#1. Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
#2. Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
#3. Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# tclint-disable
# tclfmt-disable
namespace eval pdngen {
variable block_masters {}
variable logical_viarules {}
variable physical_viarules {}
variable vias {}
variable stripe_locs
variable layers {}
variable block
variable tech
variable libs
variable design_data {}
variable default_grid_data {}
variable def_output
variable widths
variable pitches
variable loffset
variable boffset
variable site
variable row_height
variable metal_layers {}
variable blockages {}
variable padcell_blockages {}
variable instances {}
variable default_template_name {}
variable template {}
variable default_cutclass {}
variable twowidths_table {}
variable twowidths_table_wrongdirection {}
variable stdcell_area ""
variable power_nets {}
variable ground_nets {}
variable macros {}
variable verbose 0
variable global_connections {}
variable default_global_connections {
  VDD {
    {inst_name .* pin_name ^VDD$}
    {inst_name .* pin_name ^VDDPE$}
    {inst_name .* pin_name ^VDDCE$}
  }
  VSS {
    {inst_name .* pin_name ^VSS$}
    {inst_name .* pin_name ^VSSE$}
  }
}
variable power_switch_cells {}
variable psw_instance {}
variable voltage_domains {
  CORE {
    primary_power VDD primary_ground VSS
  }
}

proc check_design_state {} {
  if {[ord::get_db_block] == "NULL"} {
    utl::error PDN 9072 "Design must be loaded before calling pdngen commands."
  }
}

proc check_orientations {orientations} {
  set valid_orientations {R0 R90 R180 R270 MX MY MXR90 MYR90}
  set lef_orientations {N R0 FN MY S R180 FS MX E R270 FE MYR90 W R90 FW MXR90}

  set checked_orientations {}
  foreach orient $orientations {
    if {[lsearch -exact $valid_orientations $orient] > -1} {
      lappend checked_orientations $orient
    } elseif {[dict exists $lef_orientations $orient]} {
      lappend checked_orientations [dict get $lef_orientations $orient]
    } else {
      utl::error PDN 9074 "Invalid orientation $orient specified,\
        must be one of [join $valid_orientations {, }]."
    }
  }
  return $checked_orientations
}

proc check_layer_names {layer_names} {
  set tech [ord::get_db_tech]

  foreach layer_name $layer_names {
    if {[$tech findLayer $layer_name] == "NULL"} {
      if {[regexp {(.*)_PIN_(hor|ver)$} $layer_name - actual_layer_name]} {
        if {[$tech findLayer $actual_layer_name] == "NULL"} {
          utl::error PDN 9075 "Layer $actual_layer_name not found in loaded technology data."
        }
      } else {
        utl::error PDN 9076 "Layer $layer_name not found in loaded technology data."
      }
    }
  }
  return $layer_names
}

proc check_layer_width {layer_name width} {
  set tech [ord::get_db_tech]

  set layer [$tech findLayer $layer_name]
  set minWidth [$layer getMinWidth]
  set maxWidth [$layer getMaxWidth]

  if {[ord::microns_to_dbu $width] < $minWidth} {
    utl::error PDN 9077 "Width ($width) specified for layer $layer_name\
      is less than minimum width ([ord::dbu_to_microns $minWidth])."
  }
  if {[ord::microns_to_dbu $width] > $maxWidth} {
    utl::error PDN 9078 "Width ($width) specified for layer $layer_name\
      is greater than maximum width ([ord::dbu_to_microns $maxWidth])."
  }
  return $width
}

proc check_layer_spacing {layer_name spacing} {
  set tech [ord::get_db_tech]

  set layer [$tech findLayer $layer_name]
  set minSpacing [$layer getSpacing]

  if {[ord::microns_to_dbu $spacing] < $minSpacing} {
    utl::error PDN 9079 "Spacing ($spacing) specified for layer $layer_name is less than minimum spacing ([ord::dbu_to_microns $minSpacing)]."
  }
  return $spacing
}

proc check_rails {rails_spec} {
  if {[llength $rails_spec] % 2 == 1} {
    utl::error PDN 9081 "Expected an even number of elements in the list for -rails option, got [llength $rails_spec]."
  }
  check_layer_names [dict keys $rails_spec]
  foreach layer_name [dict keys $rails_spec] {
    if {[dict exists $rails_spec $layer_name width]} {
      check_layer_width $layer_name [dict get $rails_spec $layer_name width]
    }
    if {[dict exists $rails_spec $layer_name spacing]} {
      check_layer_spacing $layer_name [dict get $rails_spec $layer_name spacing]
    }
    if {![dict exists $rails_spec $layer_name pitch]} {
      dict set rails_spec $layer_name pitch [ord::dbu_to_microns [expr [get_row_height] * 2]]
    }
  }
  return $rails_spec
}

proc check_straps {straps_spec} {
  if {[llength $straps_spec] % 2 == 1} {
    utl::error PDN 9083 "Expected an even number of elements in the list for straps specification, got [llength $straps_spec]."
  }
  check_layer_names [dict keys $straps_spec]
  foreach layer_name [dict keys $straps_spec] {
    if {[dict exists $straps_spec $layer_name width]} {
      check_layer_width $layer_name [dict get $straps_spec $layer_name width]
    } else {
      utl::error PDN 9084 "Missing width specification for strap on layer $layer_name."
    }
    set width [ord::microns_to_dbu [dict get $straps_spec $layer_name width]]

    if {![dict exists $straps_spec $layer_name spacing]} {
      dict set straps_spec $layer_name spacing [expr [dict get $straps_spec $layer_name pitch] / 2.0]
    }
    check_layer_spacing $layer_name [dict get $straps_spec $layer_name spacing]
    set spacing [ord::microns_to_dbu [dict get $straps_spec $layer_name spacing]]

    if {[dict exists $straps_spec $layer_name pitch]} {
      set layer [[ord::get_db_tech] findLayer $layer_name]
      set minPitch [expr 2 * ([$layer getSpacing] + $width)]
      if {[ord::microns_to_dbu [dict get $straps_spec $layer_name pitch]] < $minPitch} {
        utl::error PDN 9085 "Pitch [dict get $straps_spec $layer_name pitch] specified for layer $layer_name is less than 2 x (width + spacing) (width=[ord::dbu_to_microns $width], spacing=[ord::dbu_to_microns $spacing])."
      }
    } else {
      utl::error PDN 9086 "No pitch specified for strap on layer $layer_name."
    }
  }
  return $straps_spec
}

proc check_connect {grid connect_spec} {
  foreach connect_statement $connect_spec {
    if {[llength $connect_statement] < 2} {
      utl::error PDN 9087 "Connect statement must consist of at least 2 entries."
    }
    check_layer_names [lrange $connect_statement 0 1]
    dict set layers [lindex $connect_statement 0] 1
    dict set layers [lindex $connect_statement 1] 1
  }

  if {[dict get $grid type] == "macro"} {
    set pin_layer_defined 0
    set actual_layers {}
    foreach layer_name [dict keys $layers] {
      if {[regexp {(.*)_PIN_(hor|ver)$} $layer_name - layer]} {
        lappend actual_layers $layer
      } else {
        lappend actual_layers $layer_name
      }
    }
  }
  return $connect_spec
}

proc check_core_ring {core_ring_spec} {
  if {[llength $core_ring_spec] % 2 == 1} {
    utl::error PDN 9109 "Expected an even number of elements in the list for core_ring specification, got [llength $core_ring_spec]."
  }
  set layer_directions {}
  check_layer_names [dict keys $core_ring_spec]
  foreach layer_name [dict keys $core_ring_spec] {
    if {[dict exists $core_ring_spec $layer_name width]} {
      check_layer_width $layer_name [dict get $core_ring_spec $layer_name width]
    } else {
      utl::error PDN 9121 "Missing width specification for strap on layer $layer_name."
    }
    set width [ord::microns_to_dbu [dict get $core_ring_spec $layer_name width]]

    if {![dict exists $core_ring_spec $layer_name spacing]} {
      dict set core_ring_spec $layer_name spacing [expr [dict get $core_ring_spec $layer_name pitch] / 2.0]
    }
    check_layer_spacing $layer_name [dict get $core_ring_spec $layer_name spacing]
    set spacing [ord::microns_to_dbu [dict get $core_ring_spec $layer_name spacing]]
    dict set layer_directions [get_dir $layer_name] $layer_name

    if {[dict exists $core_ring_spec $layer_name core_offset]} {
      check_layer_spacing $layer_name [dict get $core_ring_spec $layer_name core_offset]
    } elseif {[dict exists $core_ring_spec $layer_name pad_offset]} {
      check_layer_spacing $layer_name [dict get $core_ring_spec $layer_name pad_offset]
    } else {
      utl::error PDN 9146 "Must specify a pad_offset or core_offset for rings."
    }
  }
  if {[llength [dict keys $layer_directions]] == 0} {
    utl::error PDN 9139 "No direction defined for layers [dict keys $core_ring_spec]."
  } elseif {[llength [dict keys $layer_directions]] == 1} {
    set dir [dict keys $layer_directions]
    set direction [expr $dir == "ver" ? "vertical" : "horizontal"]
    set missing_direction [expr $dir == "ver" ? "horizontal" : "vertical"]

    utl::error PDN 9140 "Layers [dict keys $core_ring_spec] are both $direction, missing layer in direction $other_direction."
  } elseif {[llength [dict keys $layer_directions]] > 2} {
    utl::error PDN 9141 "Unexpected number of directions found for layers [dict keys $core_ring_spec], ([dict keys $layer_directions])."
  }

  return $core_ring_spec
}

proc check_starts_with {value} {
  if {$value != "POWER" && $value != "GROUND"} {
    utl::error PDN 9095 "Value specified for -starts_with option ($value), must be POWER or GROUND."
  }

  return $value
}

proc check_voltage_domains {domains} {
  variable voltage_domains

  foreach domain $domains {
    if {[lsearch [dict keys $voltage_domains] $domain] == -1} {
      utl::error PDN 9110 "Voltage domain $domain has not been specified, use set_voltage_domain to create this voltage domain."
    }
  }

  return $domains
}

proc check_instances {instances} {
  variable $block

  foreach instance $instances {
    if {[$block findInst $instance] == "NULL"} {
      utl::error PDN 9111 "Instance $instance does not exist in the design."
    }
  }

  return $instances
}

proc check_cells {cells} {
  foreach cell $cells {
    if {[[ord::get_db] findMaster $cell] == "NULL"} {
      utl::warn PDN 9112 "Cell $cell not loaded into the database."
    }
  }

  return $cells
}

proc check_region {region_name} {
  set block [ord::get_db_block]

  if {[$block findRegion $region_name] == "NULL"} {
    utl::error PDN 9127 "No region $region_name found in the design for voltage_domain."
  }

  return $region_name
}

proc check_switch_cells {switch_cells} {
  foreach switch_cell $switch_cells {
    if {[[ord::get_db] findMaster $switch_cell] == "NULL"} {
      utl::warn PDN 9181 "Switch cell $switch_cell not loaded into the database."
    }
  }

  return $switch_cells
}

proc check_switched_power {switched_power_net_name} {
  set block [ord::get_db_block]

  if {[set net [$block findNet $switched_power_net_name]] == "NULL"} {
    set net [odb::dbNet_create $block $switched_power_net_name]
    $net setSpecial
    $net setSigType "POWER"
  } else {
    if {[$net getSigType] != "POWER"} {
      utl::error PDN 9180 "Net $switched_power_net_name already exists in the design, but is of signal type [$net getSigType]."
    }
  }
  add_power_net $switched_power_net_name
  return $switched_power_net_name
}

proc check_power {power_net_name} {
  set block [ord::get_db_block]

  if {[set net [$block findNet $power_net_name]] == "NULL"} {
    set net [odb::dbNet_create $block $power_net_name]
    $net setSpecial
    $net setSigType "POWER"
  } else {
    if {[$net getSigType] != "POWER"} {
      utl::error PDN 9128 "Net $power_net_name already exists in the design, but is of signal type [$net getSigType]."
    }
  }
  add_power_net $power_net_name
  return $power_net_name
}

proc check_secondary_power {secondary_power_net_name} {
  add_power_net $secondary_power_net_name

  set block [ord::get_db_block]

  foreach secondary_power $secondary_power_net_name {
    if {[set net [$block findNet $secondary_power]] == "NULL"} {
      set net [odb::dbNet_create $block $secondary_power]
      $net setSpecial
      $net setSigType "POWER"
    } else {
      if {[$net getSigType] != "POWER"} {
        utl::error PDN 9176 "Net $secondary_power already exists in the design, but is of signal type [$net getSigType]."
      }
    }
  }
  return $secondary_power_net_name
}

proc check_ground {ground_net_name} {
  add_ground_net $ground_net_name

  set block [ord::get_db_block]

  if {[set net [$block findNet $ground_net_name]] == "NULL"} {
    set net [odb::dbNet_create $block $ground_net_name]
    $net setSpecial
    $net setSigType "GROUND"
  } else {
    if {[$net getSigType] != "GROUND"} {
      utl::error PDN 9129 "Net $ground_net_name already exists in the design, but is of signal type [$net getSigType]."
    }
  }
  return $ground_net_name
}

proc define_power_switch_cell {args} {
  variable power_switch_cells

  set power_switch_cell {}
  set process_args $args
  while {[llength $process_args] > 0} {
    set arg [lindex $process_args 0]
    set value [lindex $process_args 1]

    switch $arg {
      -name             {dict set power_switch_cell name $value}
      -control          {dict set power_switch_cell control $value}
      -acknowledge      {dict set power_switch_cell acknowledge $value}
      -power_switchable {dict set power_switch_cell power_switchable $value}
      -power            {dict set power_switch_cell power $value}
      -ground           {dict set power_switch_cell ground $value}
      default           {utl::error PDN 9190 "Unrecognized argument $arg, should be one of -name, -control, -acknowledge, -power, -ground."}
    }

    set process_args [lrange $process_args 2 end]
  }
  dict set power_switch_cells [dict get $power_switch_cell name] $power_switch_cell
}

proc set_voltage_domain {args} {
  variable voltage_domains

  set voltage_domain {}
  set process_args $args
  while {[llength $process_args] > 0} {

    set arg [lindex $process_args 0]
    set value [lindex $process_args 1]

    switch $arg {
      -name            {dict set voltage_domain name $value}
      -power           {dict set voltage_domain primary_power [check_power $value]}
      -switched_power  {dict set voltage_domain switched_power [check_switched_power $value]}
      -secondary_power {dict set voltage_domain secondary_power [check_secondary_power $value]}
      -ground          {dict set voltage_domain primary_ground [check_ground $value]}
      -region          {dict set voltage_domain region [check_region $value]}
      default          {utl::error PDN 9130 "Unrecognized argument $arg, should be one of -name, -power, -ground -region."}
    }

    set process_args [lrange $process_args 2 end]
  }
  dict set voltage_domains [dict get $voltage_domain name] $voltage_domain
}

proc check_direction {direction} {
  if {$direction != "horizontal" && $direction != "vertical"} {
    utl::error PDN 9138 "Unexpected value for direction ($direction), should be horizontal or vertical."
  }
  return $direction
}

proc check_number {value} {
  if {![string is double $value]} {
    error "value ($value) not recognized as a number."
  }

  return $value
}

proc check_integer {value} {
  if {![string is integer $value]} {
    error "value ($value) not recognized as a number."
  }

  return $value
}

proc check_max_rows {value} {
  foreach item $value {
    if {[catch {check_integer $item} msg]} {
      utl::error PDN 9179 "Problem with max_rows specification, $msg."
    }
  }

  return $value
}

proc check_max_columns {value} {
  foreach item $value {
    if {[catch {check_number $item} msg]} {
      utl::error PDN 9178 "Problem with max_columns specification, $msg."
    }
  }

  return $value
}

proc check_halo {value} {
  foreach item $value {
    if {[catch {check_number $item} msg]} {
      utl::error PDN 9164 "Problem with halo specification, $msg."
    }
  }

  return $value
}

proc check_net {value} {
  if {[set net [[ord::get_db_block] findNet $value]] == "NULL"} {
    utl::error PDN 9194 "Net $value does not exist in the design"
  }

  return $value
}

proc check_power_control_network_option {value} {
  if {$value == "STAR"} {
    return $value
  } elseif {$value == "DAISY"} {
    return $value
  }
  utl::error PDN 9195 "Option -power_control_network must be set to STAR or DAISY"
}

proc define_pdn_grid {args} {
  variable current_grid

  set grid {}

  set process_args $args
  while {[llength $process_args] > 0} {
    set arg [lindex $process_args 0]
    set value [lindex $process_args 1]

    switch $arg {
      -name              {dict set grid name $value}
      -voltage_domains   {dict set grid voltage_domains [check_voltage_domains $value]}
      -macro             {dict set grid type macro}
      -grid_over_pg_pins {dict set grid grid_over_pg_pins $value}
      -orient            {dict set grid orient [check_orientations $value]}
      -instances         {dict set grid instances [check_instances $value]}
      -cells             {dict set grid macro [check_cells $value]}
      -halo              {dict set grid halo [check_halo [lmap x $value {ord::microns_to_dbu [check_number $x]}]]}
      -pins              {dict set grid pins [check_layer_names $value]}
      -starts_with       {dict set grid starts_with [check_starts_with $value]}
      -pin_direction     {dict set grid pin_direction [check_direction $value]}
      -switch_cell       {dict set grid switch_cell [check_switch_cells $value]}
      -power_control     {dict set grid power_control [check_net $value]}
      -power_control_network {dict set grid power_control_network [check_power_control_network_option $value]}
      default            {utl::error PDN 9088 "Unrecognized argument $arg, should be one of -name, -orient, -instances -cells -pins -starts_with."}
    }

    set process_args [lrange $process_args 2 end]
  }

  set current_grid [verify_grid $grid]
}

proc get_grid {grid_name} {
  variable design_data

  if {[dict exists $design_data grid]} {
    dict for {type grids} [dict get $design_data grid] {
      dict for {name grid} $grids {
        if {$name == $grid_name} {
          return $grid
        }
      }
    }
  }

  return {}
}

proc check_grid {grid} {
  if {$grid == {}} {
    utl::error PDN  113 "The grid $grid_name has not been defined."
  }
  return $grid
}

proc get_power_nets {{voltage_domain ""}} {
  variable design_data
  variable voltage_domains

  if {$voltage_domain == ""} {
    set voltage_domain [dict get $design_data core_domain]
  }

  set power_nets {}
  if {[dict exists $voltage_domains $voltage_domain]} {
    if {[dict exists $voltage_domains $voltage_domain primary_power]} {
      lappend power_nets [dict get $voltage_domains $voltage_domain primary_power]
    }
    if {[dict exists $voltage_domains $voltage_domain secondary_power]} {
      lappend power_nets {*}[dict get $voltage_domains $voltage_domain secondary_power]
    }
  }
  # debug "domain: $voltage_domain, power_nets: $power_nets"

  return $power_nets
}

proc get_ground_nets {{voltage_domain ""}} {
  variable design_data
  variable voltage_domains

  if {$voltage_domain == ""} {
    set voltage_domain [dict get $design_data core_domain]
  }

  set ground_nets {}
  if {[dict exists $voltage_domains $voltage_domain]} {
    if {[dict exists $voltage_domains $voltage_domain primary_ground]} {
      lappend ground_nets [dict get $voltage_domains $voltage_domain primary_ground]
    }
  }

  return $ground_nets
}

proc get_pg_nets {{voltage_domain ""}} {
  variable design_data
  variable voltage_domains

  if {$voltage_domain == ""} {
    set voltage_domain [dict get $design_data core_domain]
  }

  set pg_nets {}
  if {[dict exists $voltage_domains $voltage_domain]} {
    if {[dict exists $voltage_domains $voltage_domain primary_power]} {
      lappend pg_nets [dict get $voltage_domains $voltage_domain primary_power]
    }
    if {[dict exists $voltage_domains $voltage_domain primary_ground]} {
      lappend pg_nets [dict get $voltage_domains $voltage_domain primary_ground]
    }
    if {[dict exists $voltage_domains $voltage_domain secondary_power]} {
      lappend pg_nets {*}[dict get $voltage_domains $voltage_domain secondary_power]
    }
    if {[dict exists $voltage_domains $voltage_domain switched_power]} {
      lappend pg_nets {*}[dict get $voltage_domains $voltage_domain switched_power]
    }
  }
   return $pg_nets
}

proc get_all_pg_nets {} {
  variable voltage_domains
  # debug "voltage_domains: $voltage_domains"

  set pg_nets {}
  dict for {voltage_domain_name voltage_domain} $voltage_domains {
    foreach net [get_pg_nets $voltage_domain_name] {
      if {[lsearch $pg_nets $net] == -1} {
        lappend pg_nets $net
      }
    }
  }

  variable power_nets
  variable ground_nets

  foreach net [concat $power_nets $ground_nets] {
    if {[lsearch $pg_nets $net] == -1} {
      lappend pg_nets $net
    }
  }

  # debug $pg_nets
  return $pg_nets
}

proc is_power_net {net} {
  variable power_nets

  if {[lsearch $power_nets $net] > -1} {
    return 1
  } else {
    return 0
  }
}

proc is_ground_net {net} {
  variable ground_nets

  if {[lsearch $ground_nets $net] > -1} {
    return 1
  } else {
    return 0
  }
}

proc get_signal_type {net_name} {
  if {[is_power_net $net_name]} {
    return "POWER"
  } elseif {[is_ground_net $net_name]} {
    return "GROUND"
  }
  return "SIGNAL"
}

proc check_power_ground {value} {
  if {$value == "POWER" || $value == "GROUND"} {
    return $value
  }
  utl::error PDN 9114 "Unexpected value ($value), must be either POWER or GROUND."
}

proc add_pdn_stripe {args} {
  variable current_grid

  if {[dict exists $args -grid]} {
    set current_grid [check_grid [get_grid [dict get $args -grid]]]
  }
  set grid $current_grid

  if {[lsearch $args -followpins] > -1} {
    set stripe rails
    set args [lreplace $args [lsearch $args "-followpins"] [lsearch $args "-followpins"]]
  } else {
    set stripe straps
  }
  set layer [check_layer_names [dict get $args -layer]]

  set process_args $args
  while {[llength $process_args] > 0} {
    set arg [lindex $process_args 0]
    set value [lindex $process_args 1]

    switch $arg {
      -grid            {;}
      -layer           {;}
      -width           {dict set grid $stripe $layer width $value}
      -spacing         {dict set grid $stripe $layer spacing $value}
      -offset          {dict set grid $stripe $layer offset $value}
      -pitch           {dict set grid $stripe $layer pitch $value}
      -starts_with     {dict set grid $stripe $layer starts_with [check_power_ground $value]}
      -extend_to_core_ring {dict set grid $stripe $layer extend_to_core_ring 1}
      stripe           {;}
      default          {utl::error PDN 9124 "Unrecognized argument $arg, should be one of -grid, -type, -orient, -power_pins, -ground_pins, -blockages, -rails, -straps, -connect."}
    }

    set process_args [lrange $process_args 2 end]
  }

  set current_grid [verify_grid $grid]
}

proc check_max_length {values max_length} {
  if {[llength $values] > $max_length} {
    error "[llength $values] provided, maximum of $max_length values allowed."
  }
}

proc check_grid_voltage_domains {grid} {
  if {![dict exists $grid voltage_domains]} {
    utl::error PDN 9158 "No voltage domains defined for grid."
  }
}

proc get_voltage_domain_by_name {domain_name} {
  variable voltage_domains

  if {[dict exists $voltage_domains $domain_name]} {
    return [dict get $voltage_domains $domain_name]
  }

  utl::error PDN 9159 "Voltage domains $domain_name has not been defined."
}

proc global_connections {args} {
  set global_connects $args
  dict for {net connect} $global_connects {
    if {[dict exists $connect inst_name]} {
      set inst_pattern [dict get $connect inst_name]
    } else {
      set inst_pattern {.*}
    }

    pdngen::add_global_connect $net $inst_pattern [dict get $connect pin_name] true
  }
}

proc match_inst_connection {inst net_name} {
  variable global_connections

  foreach pattern [dict get $global_connections $net_name] {
    if {[regexp [dict get $pattern inst_name] [$inst getName]]} {
      foreach pin [[$inst getMaster] getMTerms] {
        if {[regexp [dict get $pattern pin_name] [$pin getName]]} {
          return 1
        }
      }
    }
  }
  return 0
}

proc is_inst_in_voltage_domain {inst domain_name} {
  set voltage_domain [get_voltage_domain_by_name $domain_name]

  # The instance is in the voltage domain if it connected to both related power and ground nets
  set power_net [dict get $voltage_domain primary_power]
  set ground_net [dict get $voltage_domain primary_ground]

  return [expr {[match_inst_connection $inst $power_net] && [match_inst_connection $inst $ground_net]}]
}

proc get_block_inst_masters {} {
  variable block_masters

  if {[llength $block_masters] == 0} {
    foreach inst [[ord::get_db_block] getInsts] {
      if {[lsearch $block_masters [[$inst getMaster] getName]] == -1} {
        lappend block_masters [[$inst getMaster] getName]
      }
    }
  }
  return $block_masters
}

proc is_cell_present {cell_name} {
  return [expr {[lsearch [get_block_inst_masters] $cell_name] > -1}]
}

proc check_pwr_pads {grid cells} {
  check_grid_voltage_domains $grid
  set voltage_domains [dict get $grid voltage_domains]
  set pwr_pads {}
  set inst_example {}
  foreach voltage_domain $voltage_domains {

    set net_name [get_voltage_domain_power $voltage_domain]
    if {[set net [[ord::get_db_block] findNet $net_name]] == "NULL"} {
      utl::error PDN 9149 "Power net $net_name not found."
    }
    set find_cells $cells
    foreach inst [[ord::get_db_block] getInsts] {
      if {[set idx [lsearch $find_cells [[$inst getMaster] getName]]] > -1} {
        if {![is_inst_in_voltage_domain $inst $voltage_domain]} {continue}
        # Only need one example of each cell
        set cell_name [lindex $find_cells $idx]
        set find_cells [lreplace $find_cells $idx $idx]
        dict set inst_example $cell_name $inst
      }
      if {[llength $find_cells] == 0} {break}
    }
    if {[llength $find_cells] > 0} {
      utl::warn PDN 9150 "Cannot find cells ([join $find_cells {, }]) in voltage domain $voltage_domain."
    }
    dict for {cell inst} $inst_example {
      set pin_name [get_inst_pin_connected_to_net $inst $net]
      dict lappend pwr_pads $pin_name $cell
    }
  }

  return $pwr_pads
}

proc check_gnd_pads {grid cells} {
  check_grid_voltage_domains $grid
  set voltage_domains [dict get $grid voltage_domains]
  set gnd_pads {}
  set inst_example {}
  foreach voltage_domain $voltage_domains {
    set net_name [get_voltage_domain_ground $voltage_domain]
    if {[set net [[ord::get_db_block] findNet $net_name]] == "NULL"} {
      utl::error PDN 9151 "Ground net $net_name not found."
    }
    set find_cells $cells
    foreach inst [[ord::get_db_block] getInsts] {
      if {[set idx [lsearch $find_cells [[$inst getMaster] getName]]] > -1} {
        if {![is_inst_in_voltage_domain $inst $voltage_domain]} {continue}
        # Only need one example of each cell
        set cell_name [lindex $find_cells $idx]
        set find_cells [lreplace $find_cells $idx $idx]
        dict set inst_example $cell_name $inst
      }
      if {[llength $find_cells] == 0} {break}
    }
    if {[llength $find_cells] > 0} {
      utl::warn PDN 9152 "Cannot find cells ([join $find_cells {, }]) in voltage domain $voltage_domain."
    }
    dict for {cell inst} $inst_example {
      set pin_name [get_inst_pin_connected_to_net $inst $net]
      dict lappend gnd_pads $pin_name $cell
    }
  }
  return $gnd_pads
}

proc add_pdn_ring {args} {
  variable current_grid

  if {[dict exists $args -grid]} {
    set current_grid [check_grid [get_grid [dict get $args -grid]]]
  }
  set grid $current_grid
  set layers [check_layer_names [dict get $args -layers]]

  set process_args $args
  while {[llength $process_args] > 0} {
    set arg [lindex $process_args 0]
    set value [lindex $process_args 1]

    switch $arg {
      -grid            {;}
      -layers          {;}
      -widths {
        if {[catch {check_max_length $value 2} msg]} {
          utl::error PDN 9115 "Unexpected number of values for -widths, $msg."
        }
        if {[llength $value] == 1} {
          set values [list $value $value]
        } else {
          set values $value
        }
        foreach layer $layers width $values {
          dict set grid core_ring $layer width $width
        }
      }
      -spacings {
        if {[catch {check_max_length $value 2} msg]} {
          utl::error PDN 9116 "Unexpected number of values for -spacings, $msg."
        }
        if {[llength $value] == 1} {
          set values [list $value $value]
        } else {
          set values $value
        }
        foreach layer $layers spacing $values {
          dict set grid core_ring $layer spacing $spacing
        }
      }
      -core_offsets {
        if {[catch {check_max_length $value 2} msg]} {
          utl::error PDN 9117 "Unexpected number of values for -core_offsets, $msg."
        }
        if {[llength $value] == 1} {
          set values [list $value $value]
        } else {
          set values $value
        }
        foreach layer $layers offset $values {
          dict set grid core_ring $layer core_offset $offset
        }
      }
      -pad_offsets {
        if {[catch {check_max_length $value 2} msg]} {
          utl::error PDN 9118 "Unexpected number of values for -pad_offsets, $msg."
        }
        if {[llength $value] == 1} {
          set values [list $value $value]
        } else {
          set values $value
        }
        foreach layer $layers offset $values {
          dict set grid core_ring $layer pad_offset $offset
        }
      }
      -power_pads      {dict set grid pwr_pads [check_pwr_pads $grid $value]}
      -ground_pads     {dict set grid gnd_pads [check_gnd_pads $grid $value]}
      default          {utl::error PDN 9125 "Unrecognized argument $arg, should be one of -grid, -type, -orient, -power_pins, -ground_pins, -blockages, -rails, -straps, -connect."}
    }

    set process_args [lrange $process_args 2 end]
  }

  set current_grid [verify_grid $grid]
}

proc check_fixed_vias {via_names} {
  set tech [ord::get_db_tech]

  foreach via_name $via_names {
    if {[set via [$tech findVia $via_name]] == "NULL"} {
      utl::error PDN 9119 "Via $via_name specified in the grid specification does not exist in this technology."
    }
  }

  return $via_names
}

proc add_pdn_connect {args} {
  variable current_grid

  if {[dict exists $args -grid]} {
    set current_grid [check_grid [get_grid [dict get $args -grid]]]
  }
  set grid $current_grid

  set layers [check_layer_names [dict get $args -layers]]

  set process_args $args
  while {[llength $process_args] > 0} {
    set arg [lindex $process_args 0]
    set value [lindex $process_args 1]

    switch $arg {
      -grid            {;}
      -layers          {;}
      -cut_pitch       {dict set layers constraints cut_pitch $value}
      -fixed_vias      {dict set layers fixed_vias [check_fixed_vias $value]}
      -max_rows        {dict set layers constraints max_rows [check_max_rows $value]}
      -max_columns     {dict set layers constraints max_columns [check_max_columns $value]}
      -ongrid          {dict set layers constraints ongrid [check_layer_names $value]}
      -split_cuts      {dict set layers constraints split_cuts [check_layer_names $value]}
      default          {utl::error PDN 9126 "Unrecognized argument $arg, should be one of -grid, -type, -orient, -power_pins, -ground_pins, -blockages, -rails, -straps, -connect."}
    }

    set process_args [lrange $process_args 2 end]
  }

  dict lappend grid connect $layers
  set current_grid [verify_grid $grid]
}

proc convert_grid_to_def_units {grid} {
  if {![dict exists $grid units]} {
    if {[dict exists $grid core_ring]} {
      dict for {layer data} [dict get $grid core_ring] {
        dict set grid core_ring $layer [convert_layer_spec_to_def_units $data]
      }
    }

    if {[dict exists $grid rails]} {
      dict for {layer data} [dict get $grid rails] {
        dict set grid rails $layer [convert_layer_spec_to_def_units $data]
        if {[dict exists $grid template]} {
          foreach template [dict get $grid template names] {
            if {[dict exists $grid layers $layer $template]} {
              dict set grid rails $layer $template [convert_layer_spec_to_def_units [dict get $grid rails $layer $template]]
            }
          }
        }
      }
    }
    if {[dict exists $grid straps]} {
      dict for {layer data} [dict get $grid straps] {
        dict set grid straps $layer [convert_layer_spec_to_def_units $data]
        if {[dict exists $grid template]} {
          foreach template [dict get $grid template names] {
            if {[dict exists $grid straps $layer $template]} {
              dict set grid straps $layer $template [convert_layer_spec_to_def_units [dict get $grid straps $layer $template]]
            }
          }
        }
      }
    }
    dict set grid units "db"
  }

  return $grid
}

proc get_inst_pin_connected_to_net {inst net} {
  foreach iterm [$inst getITerms] {
    # debug "[$inst getName] [$iterm getNet] == $net"
    if {[$iterm getNet] == $net} {
      return [[$iterm getMTerm] getName]
    }
  }
}

proc filter_out_selected_by {instances selection} {
  dict for {inst_name instance} $instances {
    if {[dict exists $instance selected_by]} {
      if {[dict get $instance selected_by] == $selection} {
        set instances [dict remove $instances $inst_name]
      }
    }
  }

  return $instances
}

proc get_priority_value {priority} {
  if {$priority == "inst_name"} {return 4}
  if {$priority == "cell_name"} {return 3}
  if {$priority == "orient"} {return 2}
  if {$priority == "none"} {return 1}
}

proc set_instance_grid {inst_name grid priority} {
  variable instances

  # debug "start- inst_name $inst_name, grid: [dict get $grid name], priority: $priority"
  set grid_name [dict get $grid name]
  set priority_value [get_priority_value $priority]
  set instance [dict get $instances $inst_name]
  if {[dict exists $instance grid]} {
    if {[dict get $instance grid] != $grid_name} {
      set current_priority_value [get_priority_value [dict get $instance selected_by]]
      if {$priority_value < $current_priority_value} {
        return
      } elseif {$priority_value == $current_priority_value} {
        utl::error PDN 9165 "Conflict found, instance $inst_name is part of two grid definitions ($grid_name, [dict get $instances $inst_name grid])."
      }
    }
  } else {
    dict set instances $inst_name grid $grid_name
  }

  if {[dict exists $grid halo]} {
    set_instance_halo $inst_name [dict get $grid halo]
  }
  dict set instances $inst_name selected_by $priority
  dict set instances $inst_name grid $grid_name
  dict set insts $inst_name selected_by $priority
  dict set insts $inst_name grid $grid_name
}

proc verify_grid {grid} {
  variable design_data
  variable default_grid_data

  if {![dict exists $grid type]} {
    dict set grid type stdcell
  }
  set type [dict get $grid type]

  if {![dict exists $grid voltage_domains]} {
    dict set grid voltage_domains "CORE"
  }
  set voltage_domains [dict get $grid voltage_domains]

  if {![dict exists $grid name]} {
    set idx 1
    set name "[join [dict get $grid voltage_domains] {_}]_${type}_grid_$idx"
    while {[get_grid $name] != {}} {
      incr idx
      set name "[join [dict get $grid voltage_domains] {_}]_${type}_grid_$idx"
    }
    dict set grid name $name
  }
  set grid_name [dict get $grid name]

  if {[dict exists $grid core_ring]} {
    check_core_ring [dict get $grid core_ring]
    set layer [lindex [dict keys [dict get $grid core_ring]]]
    if {[dict exist $grid core_ring $layer pad_offset]} {
      if {![dict exists $grid pwr_pads]} {
        utl::error PDN 9147 "No definition of power padcells provided, required when using pad_offset."
      }
      if {![dict exists $grid gnd_pads]} {
        utl::error PDN 9148 "No definition of ground padcells provided, required when using pad_offset."
      }
    }
  }

  if {[dict exists $grid pwr_pads]} {
    dict for {pin_name cells} [dict get $grid pwr_pads] {
      foreach cell $cells {
        if {[set master [[ord::get_db] findMaster $cell]] == "NULL"} {
          utl::error PDN 9153  "Core power padcell ($cell) not found in the database."
        }
        if {[$master findMTerm $pin_name] == "NULL"} {
          utl::error PDN 9154 "Cannot find pin ($pin_name) on core power padcell ($cell)."
        }
      }
    }
  }

  if {[dict exists $grid gnd_pads]} {
    dict for {pin_name cells} [dict get $grid gnd_pads] {
      foreach cell $cells {
        if {[set master [[ord::get_db] findMaster $cell]] == "NULL"} {
          utl::error PDN 9155  "Core ground padcell ($cell) not found in the database."
        }
        if {[$master findMTerm $pin_name] == "NULL"} {
          utl::error PDN 9156 "Cannot find pin ($pin_name) on core ground padcell ($cell)."
        }
      }
    }
  }

  if {[dict exists $grid macro]} {
    check_cells [dict get $grid macro]
  }

  if {[dict exists $grid rails]} {
    dict set grid rails [check_rails [dict get $grid rails]]
  }

  if {[dict exists $grid straps]} {
    check_straps [dict get $grid straps]
  }

  if {[dict exists $grid template]} {
    set_template_size {*}[dict get $grid template size]
  }

  if {[dict exists $grid orient]} {
    if {$type == "stdcell"} {
      utl::error PDN 9090 "The orient attribute cannot be used with stdcell grids."
    }
    dict set grid orient [check_orientations [dict get $grid orient]]
  }

  if {[dict exists $grid connect]} {
    dict set grid connect [check_connect $grid [dict get $grid connect]]
  }

  if {$type == "macro"} {
    if {![dict exists $grid halo]} {
      dict set grid halo [get_default_halo]
    }
    check_halo [dict get $grid halo]
  } else {
    set default_grid_data $grid
  }

  # debug $grid

  dict set design_data grid $type $grid_name $grid
  return $grid
}

proc complete_macro_grid_specifications {} {
  variable design_data
  variable instances
  variable macros

  set macros [get_macro_blocks]

  dict for {type grid_types} [dict get $design_data grid] {
    dict for {name grid} $grid_types {
      dict set design_data grid $type $name [convert_grid_to_def_units $grid]
    }
  }
  if {![dict exists $design_data grid macro]} {
    return
  }

  ########################################
  # Creating blockages based on macro locations
  #######################################
  # debug "import_macro_boundaries"
  import_macro_boundaries

  # Associate each block instance with a grid specification
  set macro_names [dict keys $macros]
  dict for {grid_name grid} [dict get $design_data grid macro] {
    set insts [find_instances_of $macro_names]
    set boundary [odb::newSetFromRect {*}[get_core_area]]
    set insts [filtered_insts_within $insts $boundary]
    if {[dict exists $grid instances]} {
      # debug "Check macro name for [dict get $grid name]"
      dict for {inst_name instance} $insts {
        if {[lsearch [dict get $grid instances] $inst_name] > -1} {
          set_instance_grid $inst_name $grid inst_name
        }
      }
      set insts [set_instance_grid $selected_insts $grid inst_name]
    } elseif {[dict exists $grid macro]} {
      # set insts [filter_out_selected_by $insts inst_name]
      # debug "Check instance name for [dict get $grid name]"
      dict for {inst_name instance} $insts {
        set cell_name [dict get $instance macro]
        if {[lsearch [dict get $grid macro] $cell_name] > -1} {
          set_instance_grid $inst_name $grid cell_name
        }
      }
    } elseif {[dict exists $grid orient]} {
      # set insts [filter_out_selected_by $insts inst_name]
      # set insts [filter_out_selected_by $insts cell_name]
      # debug "Check orientation for [dict get $grid name]"
      dict for {inst_name instance} $insts {
        set orient [dict get $instance orient]
        # debug "Inst: $inst_name, orient: $orient, compare to: [dict get $grid orient]"
        if {[lsearch [dict get $grid orient] $orient] > -1} {
          set_instance_grid $inst_name $grid orient
        }
      }
    }
  }
  dict for {grid_name grid} [dict get $design_data grid macro] {
    set related_instances {}
    dict for {inst instance} $instances {
      if {![dict exists $instance grid]} {
        # utl::error PDN 9166 "Instance $inst of cell [dict get $instance macro] is not associated with any grid."
        dict set instance grid "__none__"
      }
      if {[dict get $instance grid] == $grid_name} {
        dict set related_instances $inst $instance
      }
    }
    dict set design_data grid macro $grid_name _related_instances $related_instances
  }

  dict for {grid_name grid} [dict get $design_data grid macro] {
    # Set the pin layer on the connect statement to the pin layer of the def to be _PIN_<dir>
    set blockages {}
    set pin_layers {}
    set power_pins {}
    set ground_pins {}
    dict for {instance_name instance} [dict get $grid _related_instances] {
      lappend blockages {*}[dict get $macros [dict get $instance macro] blockage_layers]
      lappend pin_layers {*}[dict get $macros [dict get $instance macro] pin_layers]
      lappend power_pins {*}[dict get $macros [dict get $instance macro] power_pins]
      lappend ground_pins {*}[dict get $macros [dict get $instance macro] ground_pins]
    }
    dict set design_data grid macro $grid_name power_pins [lsort -unique $power_pins]
    dict set design_data grid macro $grid_name ground_pins [lsort -unique $ground_pins]

    if {[dict exists $grid pin_direction]} {
      if {[dict get $grid pin_direction] == "vertical"} {
        set direction ver
      } else {
        set direction hor
      }
      set pin_layers [lsort -unique $pin_layers]

      foreach pin_layer $pin_layers {
        set new_connections {}
        foreach connect [dict get $grid connect] {
          if {[lindex $connect 0] == $pin_layer} {
            set connect [lreplace $connect 0 0 ${pin_layer}_PIN_$direction]
          }
          if {[lindex $connect 1] == $pin_layer} {
            set connect [lreplace $connect 1 1 ${pin_layer}_PIN_$direction]
          }
          lappend new_connections $connect
        }
        dict set design_data grid macro $grid_name connect $new_connections
      }
    }

    if {[dict exists $grid straps]} {
      foreach strap_layer [dict keys [dict get $grid straps]] {
        lappend blockages $strap_layer
      }
    }
    # debug "Grid: $grid_name"
    # debug "  instances: [dict keys [dict get $grid _related_instances]]"
    # debug "  blockages: [lsort -unique $blockages]"
    # debug "  connect: [dict get $design_data grid macro $grid_name connect]"

    dict set design_data grid macro $grid_name blockages [lsort -unique $blockages]
  }

  # debug "get_memory_instance_pg_pins"
  get_memory_instance_pg_pins
}

#This file contains procedures that are used for PDN generation
proc debug {message} {
  set state [info frame -1]
  set str ""
  if {[dict exists $state file]} {
    set str "$str[dict get $state file]:"
  }
  if {[dict exists $state proc]} {
    set str "$str[dict get $state proc]:"
  }
  if {[dict exists $state line]} {
    set str "$str[dict get $state line]"
  }
  puts "\[DEBUG\] $str: $message"
}

proc lmap {args} {
  set result {}
  set var [lindex $args 0]
  foreach item [lindex $args 1] {
    uplevel 1 "set $var $item"
    lappend result [uplevel 1 [lindex $args end]]
  }
  return $result
}

proc get_routing_direction {layer_name} {
  variable layers

  if {$layers == ""} {
    init_metal_layers
  }

  if {![dict exists $layers $layer_name direction]} {
    utl::error PDN 9033 "Unknown direction for layer $layer_name."
  }
  return [dict get $layers $layer_name direction]
}

proc get_dir {layer_name} {
  if {[regexp {.*_PIN_(hor|ver)} $layer_name - dir]} {
    return $dir
  }

  if {[is_rails_layer $layer_name]} {
    return "hor"
  }

  return [get_routing_direction $layer_name]
}

proc get_rails_layers {} {
  variable design_data

  if {[dict exists $design_data grid]} {
    foreach type [dict keys [dict get $design_data grid]] {
      dict for {name specification} [dict get $design_data grid $type] {
        if {[dict exists $specification rails]} {
          return [dict keys [dict get $specification rails]]
        }
      }
    }
  }
  return {}
}

proc is_rails_layer {layer} {
  return [expr {[lsearch -exact [get_rails_layers] $layer] > -1}]
}

proc via_number {layer_rule1 layer_rule2} {
  return [expr [[$layer_rule1 getLayer] getNumber] - [[$layer_rule2 getLayer] getNumber]]
}

proc init_via_tech {} {
  variable tech
  variable def_via_tech

  set def_via_tech {}
  foreach via_rule [$tech getViaGenerateRules] {
    set levels [list [$via_rule getViaLayerRule 0] [$via_rule getViaLayerRule 1] [$via_rule getViaLayerRule 2]]
    set levels [lsort -command via_number $levels]
    lassign $levels lower cut upper

    dict set def_via_tech [$via_rule getName] [list \
      lower [list layer [[$lower getLayer] getName] enclosure [$lower getEnclosure]] \
      upper [list layer [[$upper getLayer] getName] enclosure [$upper getEnclosure]] \
      cut   [list layer [[$cut getLayer] getName] spacing [$cut getSpacing] size [list [[$cut getRect] dx] [[$cut getRect] dy]]] \
    ]
  }
  # debug "def_via_tech: $def_via_tech"
}

proc set_prop_lines {obj prop_name} {
  variable prop_line
  if {[set prop [::odb::dbStringProperty_find $obj $prop_name]] != "NULL"} {
    set prop_line [$prop getValue]
  } else {
    set prop_line {}
  }
}

proc read_propline {} {
  variable prop_line

  set word [lindex $prop_line 0]
  set prop_line [lrange $prop_line 1 end]

  set line {}
  while {[llength $prop_line] > 0 && $word != ";"} {
    lappend line $word
    set word [lindex $prop_line 0]
    set prop_line [lrange $prop_line 1 end]
  }
  return $line
}

proc empty_propline {} {
  variable prop_line
  return [expr ![llength $prop_line]]
}

proc find_layer {layer_name} {
  variable tech

  if {[set layer [$tech findLayer $layer_name]] == "NULL"} {
    utl::error PDN 9019 "Cannot find layer $layer_name in loaded technology."
  }
  return $layer
}

proc read_spacing {layer_name} {
  variable layers

  set layer [find_layer $layer_name]

  set_prop_lines $layer LEF58_SPACING
  set spacing {}

  while {![empty_propline]} {
    set line [read_propline]
    if {[set idx [lsearch -exact $line CUTCLASS]] > -1} {
      set cutclass [lindex $line [expr $idx + 1]]
      set line [lreplace $line $idx [expr $idx + 1]]

      if {[set idx [lsearch -exact $line LAYER]] > -1} {
        set other_layer [lindex $line [expr $idx + 1]]
        set line [lreplace $line $idx [expr $idx + 1]]

        if {[set idx [lsearch -exact $line CONCAVECORNER]] > -1} {
          set line [lreplace $line $idx $idx]

          if {[set idx [lsearch -exact $line SPACING]] > -1} {
            dict set spacing $cutclass $other_layer concave [ord::microns_to_dbu [lindex $line [expr $idx + 1]]]
            # set line [lreplace $line $idx [expr $idx + 1]]
          }
        }
      }
    }
  }
  # debug "$layer_name $spacing"
  dict set layers $layer_name spacing $spacing
  # debug "$layer_name [dict get $layers $layer_name]"
}

proc read_spacingtables {layer_name} {
  variable layers

  set layer [find_layer $layer_name]
  set prls {}

  if {[$layer hasTwoWidthsSpacingRules]} {
    set type "TWOWIDTHS"
    set subtype "NONE"

    set table_size [$layer getTwoWidthsSpacingTableNumWidths]
    for {set i 0} {$i < $table_size} {incr i} {
      set width [$layer getTwoWidthsSpacingTableWidth $i]

      if {[$layer getTwoWidthsSpacingTableHasPRL $i]} {
        set prl [$layer getTwoWidthsSpacingTablePRL $i]
      } else {
        set prl 0
      }
      set spacings {}
      for {set j 0} {$j < $table_size} {incr j} {
        lappend spacings [$layer getTwoWidthsSpacingTableEntry $i $j]
      }

      dict set layers $layer_name spacingtable $type $subtype $width [list prl $prl spacings $spacings]
    }
  }

  set_prop_lines $layer LEF58_SPACINGTABLE
  set spacing {}

  while {![empty_propline]} {
    set line [read_propline]
    # debug "$line"
    set type [lindex $line 1]
    set subtype [lindex $line 2]

    set table_entry_indexes [lsearch -exact -all $line "WIDTH"]
    set num_entries [llength $table_entry_indexes]

    foreach start_index $table_entry_indexes {
      set pos $start_index
      incr pos
      set width [ord::microns_to_dbu [lindex $line $pos]]
      incr pos
      if {[lindex $line $pos] == "PRL"} {
        incr pos
        set prl [ord::microns_to_dbu [lindex $line $pos]]
        incr pos
      } else {
        set prl 0
      }
      set spacings {}
      for {set i 0} {$i < $num_entries} {incr i} {
        # debug "[expr $i + $pos] [lindex $line [expr $i + $pos]]"
        lappend spacings [ord::microns_to_dbu [lindex $line [expr $i + $pos]]]
      }
      dict set layers $layer_name spacingtable $type $subtype $width [list prl $prl spacings $spacings]
    }
  }

  if {![dict exists $layers $layer_name spacingtable]} {
    dict set layers $layer_name spacingtable {}
  }
  # debug "$layer_name [dict get $layers $layer_name]"
}

proc get_spacingtables {layer_name} {
  variable layers

  if {![dict exists $layers $layer_name spacingtable]} {
    read_spacingtables $layer_name
  }

  return [dict get $layers $layer_name spacingtable]
}

proc get_concave_spacing_value {layer_name other_layer_name} {
  variable layers
  variable default_cutclass

  if {![dict exists $layers $layer_name spacing]} {
    read_spacing $layer_name
  }
  # debug "$layer_name [dict get $layers $layer_name]"
  if {[dict exists $layers $layer_name spacing [dict get $default_cutclass $layer_name] $other_layer_name concave]} {
    return [dict get $layers $layer_name spacing [dict get $default_cutclass $layer_name] $other_layer_name concave]
  }
  return 0
}

proc read_arrayspacing {layer_name} {
  variable layers

  set layer [find_layer $layer_name]

  set_prop_lines $layer LEF58_ARRAYSPACING
  set arrayspacing {}

  while {![empty_propline]} {
    set line [read_propline]
    if {[set idx [lsearch -exact $line PARALLELOVERLAP]] > -1} {
      dict set arrayspacing paralleloverlap 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line LONGARRAY]] > -1} {
      dict set arrayspacing longarray 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line CUTSPACING]] > -1} {
      dict set arrayspacing cutspacing [ord::microns_to_dbu [lindex $line [expr $idx + 1]]]
      set line [lreplace $line $idx [expr $idx + 1]]
    }
    while {[set idx [lsearch -exact $line ARRAYCUTS]] > -1} {
      dict set arrayspacing arraycuts [lindex $line [expr $idx + 1]] spacing [ord::microns_to_dbu [lindex $line [expr $idx + 3]]]
      set line [lreplace $line $idx [expr $idx + 3]]
    }
  }
  dict set layers $layer_name arrayspacing $arrayspacing
}

proc read_cutclass {layer_name} {
  variable layers
  variable default_cutclass

  set layer [find_layer $layer_name]
  set_prop_lines $layer LEF58_CUTCLASS
  dict set layers $layer_name cutclass {}
  set min_area -1

  while {![empty_propline]} {
    set line [read_propline]
    if {![regexp {CUTCLASS\s+([^\s]+)\s+WIDTH\s+([^\s]+)} $line - cut_class width]} {
      utl::error PDN 9020 "Failed to read CUTCLASS property '$line'."
    }
    if {[regexp {LENGTH\s+([^\s]+)} $line - length]} {
      set area [expr $width * $length]
    } else {
      set area [expr $width * $width]
    }
    if {$min_area == -1 || $area < $min_area} {
      dict set default_cutclass $layer_name $cut_class
      set min_area $area
    }
    dict set layers $layer_name cutclass $cut_class [list width [ord::microns_to_dbu $width] length [ord::microns_to_dbu $length]]
  }
}

proc read_enclosures {layer_name} {
  variable layers

  set layer [find_layer $layer_name]
  set_prop_lines $layer LEF58_ENCLOSURE
  set prev_cutclass ""

  while {![empty_propline]} {
    set line [read_propline]
    # debug "$line"
    set enclosure {}
    if {[set idx [lsearch -exact $line EOL]] > -1} {
      continue
      dict set enclosure eol [ord::microns_to_dbu [lindex $line [expr $idx + 1]]]
      set line [lreplace $line $idx [expr $idx + 1]]
    }
    if {[set idx [lsearch -exact $line EOLONLY]] > -1} {
      dict set enclosure eolonly 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line SHORTEDGEONEOL]] > -1} {
      dict set enclosure shortedgeoneol 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line MINLENGTH]] > -1} {
      dict set enclosure minlength [ord::microns_to_dbu [lindex $line [expr $idx + 1]]]
      set line [lreplace $line $idx [expr $idx + 1]]
    }
    if {[set idx [lsearch -exact $line ABOVE]] > -1} {
      dict set enclosure above 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line BELOW]] > -1} {
      dict set enclosure below 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line END]] > -1} {
      dict set enclosure end 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line SIDE]] > -1} {
      dict set enclosure side 1
      set line [lreplace $line $idx $idx]
    }

    set width 0
    regexp {WIDTH\s+([^\s]+)} $line - width
    set width [ord::microns_to_dbu $width]

    if {![regexp {ENCLOSURE CUTCLASS\s+([^\s]+)\s+([^\s]+)\s+([^\s]+)} $line - cut_class overlap1 overlap2]} {
      utl::error PDN 9021 "Failed to read ENCLOSURE property '$line'."
    }
    dict set enclosure overlap1 [ord::microns_to_dbu $overlap1]
    dict set enclosure overlap2 [ord::microns_to_dbu $overlap2]
    # debug "class - $cut_class enclosure - $enclosure"
    if {$prev_cutclass != $cut_class} {
      set enclosures {}
      set prev_cutclass $cut_class
    }
    dict lappend enclosures $width $enclosure
    dict set layers $layer_name cutclass $cut_class enclosures $enclosures
  }
  # debug "end"
}

proc read_minimumcuts {layer_name} {
  variable layers
  variable default_cutclass

  set layer [find_layer $layer_name]
  set_prop_lines $layer LEF58_MINIMUMCUT

  while {![empty_propline]} {
    set line [read_propline]
    set classes {}
    set constraints {}
    set fromabove 0
    set frombelow 0

    if {[set idx [lsearch -exact $line FROMABOVE]] > -1} {
      set fromabove 1
      set line [lreplace $line $idx $idx]
    } elseif {[set idx [lsearch -exact $line FROMBELOW]] > -1} {
      set frombelow 1
      set line [lreplace $line $idx $idx]
    } else {
      set fromabove 1
      set frombelow 1
    }

    if {[set idx [lsearch -exact $line WIDTH]] > -1} {
      set width [ord::microns_to_dbu [lindex $line [expr $idx + 1]]]
    }

    if {[regexp {LENGTH ([0-9\.]*) WITHIN ([0-9\.]*)} $line - length within]} {
      # Not expecting to deal with this king of structure, so can ignore
      set line [regsub {LENGTH ([0-9\.]*) WITHIN ([0-9\.]*)} $line {}]
    }

    if {[regexp {AREA ([0-9\.]*) WITHIN ([0-9\.]*)} $line - area within]} {
      # Not expecting to deal with this king of structure, so can ignore
      set line [regsub {AREA ([0-9\.]*) WITHIN ([0-9\.]*)} $line {}]
    }

    while {[set idx [lsearch -exact $line CUTCLASS]] > -1} {
      set cutclass [lindex $line [expr $idx + 1]]
      set num_cuts [lindex $line [expr $idx + 2]]

      if {$fromabove == 1} {
        dict set layers $layer_name minimumcut width $width fromabove $cutclass $num_cuts
      }
      if {$frombelow == 1} {
        dict set layers $layer_name minimumcut width $width frombelow $cutclass $num_cuts
      }

      set line [lreplace $line $idx [expr $idx + 2]]
    }
  }
}

proc get_minimumcuts {layer_name width from cutclass} {
  variable layers
  # debug "$layer_name, $width, $from, $cutclass"
  if {![dict exists $layers $layer_name minimumcut]} {
    read_minimumcuts $layer_name
    # debug "[dict get $layers $layer_name minimumcut]"
  }

  set min_cuts 1

  if {![dict exists $layers $layer_name minimumcut]} {
    # debug "No mincut rule for layer $layer_name"
    return $min_cuts
  }

  set idx 0
  set widths [lsort -integer -decreasing [dict keys [dict get $layers $layer_name minimumcut width]]]
  if {$width <= [lindex $widths end]} {
    # debug "width $width less than smallest width boundary [lindex $widths end]"
    return $min_cuts
  }
  foreach width_boundary [lreverse $widths] {
    if {$width > $width_boundary && [dict exists $layers $layer_name minimumcut width $width_boundary $from]} {
      # debug "[dict get $layers $layer_name minimumcut width $width_boundary]"
      if {[dict exists $layers $layer_name minimumcut width $width_boundary $from $cutclass]} {
        set min_cuts [dict get $layers $layer_name minimumcut width $width_boundary $from $cutclass]
      }
      # debug "Selected width boundary $width_boundary for $layer_name, $width $from, $cutclass [dict get $layers $layer_name minimumcut width $width_boundary $from $cutclass]"
      break
    }
  }

  return $min_cuts
}

proc get_via_enclosure {via_info lower_width upper_width} {
  variable layers
  variable default_cutclass
  variable min_lower_enclosure
  variable max_lower_enclosure
  variable min_upper_enclosure
  variable max_upper_enclosure

  # debug "via_info $via_info width $lower_width,$upper_width"
  set layer_name [dict get $via_info cut layer]

  if {![dict exists $layers $layer_name cutclass]} {
    read_cutclass $layer_name
    read_enclosures $layer_name
  }

  if {!([dict exists $default_cutclass $layer_name] && [dict exists $layers $layer_name cutclass [dict get $default_cutclass $layer_name] enclosures])} {
    set lower_enclosure [dict get $via_info lower enclosure]
    set upper_enclosure [dict get $via_info upper enclosure]

    set min_lower_enclosure [lindex $lower_enclosure 0]
    set max_lower_enclosure [lindex $lower_enclosure 1]

    if {$max_lower_enclosure < $min_lower_enclosure} {
      set swap $min_lower_enclosure
      set min_lower_enclosure $max_lower_enclosure
      set max_lower_enclosure $swap
    }

    set min_upper_enclosure [lindex $upper_enclosure 0]
    set max_upper_enclosure [lindex $upper_enclosure 1]

    if {$max_upper_enclosure < $min_upper_enclosure} {
      set swap $min_upper_enclosure
      set min_upper_enclosure $max_upper_enclosure
      set max_upper_enclosure $swap
    }

    set selected_enclosure [list $min_lower_enclosure $max_lower_enclosure $min_upper_enclosure $max_upper_enclosure]
  } else {
    set enclosures [dict get $layers $layer_name cutclass [dict get $default_cutclass $layer_name] enclosures]
    # debug "Enclosure set $enclosures"
    set upper_enclosures {}
    set lower_enclosures {}

    set width $lower_width

    foreach size [lreverse [dict keys $enclosures]] {
      if {$width >= $size} {
          break
      }
    }

    set enclosure_list [dict get $enclosures $size]
    # debug "Initial enclosure_list (size = $size)- $enclosure_list"
    if {$size > 0} {
      foreach enclosure $enclosure_list {
        if {![dict exists $enclosure above]} {
          lappend lower_enclosures $enclosure
        }
      }
    }

    set width $upper_width

    foreach size [lreverse [dict keys $enclosures]] {
      if {$width >= $size} {
          break
      }
    }

    set enclosure_list [dict get $enclosures $size]
    # debug "Initial enclosure_list (size = $size)- $enclosure_list"
    if {$size > 0} {
      foreach enclosure $enclosure_list {
        if {![dict exists $enclosure below]} {
          lappend upper_enclosures $enclosure
        }
      }
    }

    if {[llength $upper_enclosures] == 0} {
      set zero_enclosures_list [dict get $enclosures 0]
      foreach enclosure $zero_enclosures_list {
        if {![dict exists $enclosure below]} {
          lappend upper_enclosures $enclosure
        }
      }
    }
    if {[llength $lower_enclosures] == 0} {
      set zero_enclosures_list [dict get $enclosures 0]
      foreach enclosure $zero_enclosures_list {
        if {![dict exists $enclosure above]} {
          lappend lower_enclosures $enclosure
        }
      }
    }
    set upper_min -1
    set lower_min -1
    if {[llength $upper_enclosures] > 1} {
      foreach enclosure $upper_enclosures {
        # debug "upper enclosure - $enclosure"
        set this_min [expr min([dict get $enclosure overlap1], [dict get $enclosure overlap2])]
        if {$upper_min < 0 || $this_min < $upper_min} {
          set upper_min $this_min
          set upper_enc [list [dict get $enclosure overlap1] [dict get $enclosure overlap2]]
          # debug "upper_enc: $upper_enc"
        }
      }
    } else {
      set enclosure [lindex $upper_enclosures 0]
      set upper_enc [list [dict get $enclosure overlap1] [dict get $enclosure overlap2]]
    }
    if {[llength $lower_enclosures] > 1} {
      foreach enclosure $lower_enclosures {
        # debug "lower enclosure - $enclosure"
        set this_min [expr min([dict get $enclosure overlap1], [dict get $enclosure overlap2])]
        if {$lower_min < 0 || $this_min < $lower_min} {
          set lower_min $this_min
          set lower_enc [list [dict get $enclosure overlap1] [dict get $enclosure overlap2]]
        }
      }
      # debug "[llength $lower_enclosures] lower_enc: $lower_enc"
    } else {
      set enclosure [lindex $lower_enclosures 0]
      set lower_enc [list [dict get $enclosure overlap1] [dict get $enclosure overlap2]]
      # debug "1 lower_enc: lower_enc: $lower_enc"
    }
    set selected_enclosure [list {*}$lower_enc {*}$upper_enc]
  }
  # debug "selected $selected_enclosure"
  set min_lower_enclosure [expr min([lindex $selected_enclosure 0], [lindex $selected_enclosure 1])]
  set max_lower_enclosure [expr max([lindex $selected_enclosure 0], [lindex $selected_enclosure 1])]
  set min_upper_enclosure [expr min([lindex $selected_enclosure 2], [lindex $selected_enclosure 3])]
  set max_upper_enclosure [expr max([lindex $selected_enclosure 2], [lindex $selected_enclosure 3])]
  # debug "enclosures - min_lower $min_lower_enclosure max_lower $max_lower_enclosure min_upper $min_upper_enclosure max_upper $max_upper_enclosure"
}

proc select_via_info {lower} {
  variable def_via_tech

  set layer_name $lower
  regexp {(.*)_PIN} $lower - layer_name

  return [dict filter $def_via_tech script {rule_name rule} {expr {[dict get $rule lower layer] == $layer_name}}]
}

proc set_layer_info {layer_info} {
  variable layers

  set layers $layer_info
}

proc read_widthtable {layer_name} {
  variable tech

  set table {}
  set layer [find_layer $layer_name]
  set_prop_lines $layer LEF58_WIDTHTABLE

  while {![empty_propline]} {
    set line [read_propline]
    set flags {}
    if {[set idx [lsearch -exact $line ORTHOGONAL]] > -1} {
      dict set flags orthogonal 1
      set line [lreplace $line $idx $idx]
    }
    if {[set idx [lsearch -exact $line WRONGDIRECTION]] > -1} {
      dict set flags wrongdirection 1
      set line [lreplace $line $idx $idx]
    }

    regexp {WIDTHTABLE\s+(.*)} $line - widthtable
    set widthtable [lmap x $widthtable {ord::microns_to_dbu $x}]

    if {[dict exists $flags wrongdirection]} {
      dict set table wrongdirection $widthtable
    } else {
      dict set table rightdirection $widthtable
    }
  }
  return $table
}

proc get_widthtable {layer_name direction} {
  variable layers

  if {![dict exists $layers $layer_name widthtable]} {
      dict set layers $layer_name widthtable [read_widthtable $layer_name]
  }

  if {![dict exists $layers $layer_name widthtable $direction]} {
    if {$direction == "wrongdirection" && [dict exists $layers $layer_name widthtable rightdirection]} {
      dict set layers $layer_name widthtable $direction [dict get $layers $layer_name widthtable rightdirection]
    } else {
      dict set layers $layer_name widthtable $direction {}
    }
  }

  return [dict get $layers $layer_name widthtable $direction]
}

# Layers that have a widthtable will only support some width values, the widthtable defines the
# set of widths that are allowed, or any width greater than or equal to the last value in the
# table
#

proc adjust_width {widthtable width} {
  if {[llength $widthtable] == 0} {return $width}
  if {[lsearch -exact $widthtable $width] > -1} {return $width}
  if {$width > [lindex $widthtable end]} {return $width}

  foreach value $widthtable {
    if {$value > $width} {
      # debug "Adjust width from $width to $value"
      return $value
    }
  }

  return $width
}

proc get_adjusted_dX {layer width} {
  if {[get_routing_direction $layer] == "ver"} {
    # debug "Using rightdirection adjustment for layer $layer (dX)"
    return [adjust_width [get_widthtable $layer rightdirection] $width]
  } else {
    # debug "Using wrongdirection adjustment for layer $layer (dX)"
    return [adjust_width [get_widthtable $layer wrongdirection] $width]
  }
}

proc get_adjusted_dY {layer height} {
  if {[get_routing_direction $layer] == "hor"} {
    # debug "Using rightdirection adjustment for layer $layer (dY)"
    return [adjust_width [get_widthtable $layer rightdirection] $height]
  } else {
    # debug "Using wrongdirection adjustment for layer $layer (dY)"
    return [adjust_width [get_widthtable $layer wrongdirection] $height]
  }
}

proc get_arrayspacing_rule {layer_name} {
  variable layers

  if {![dict exists $layers $layer_name arrayspacing]} {
    read_arrayspacing $layer_name
  }

  return [dict get $layers $layer_name arrayspacing]
}

proc use_arrayspacing {layer_name rows columns} {
  set arrayspacing [get_arrayspacing_rule $layer_name]
  # debug "$arrayspacing"
  # debug "$rows $columns"
  if {[llength $arrayspacing] == 0} {
    # debug "No array spacing rule defined"
    return 0
  }
  # debug "[dict keys [dict get $arrayspacing arraycuts]]"
  if {[dict exists $arrayspacing arraycuts [expr min($rows,$columns)]]} {
    # debug "Matching entry in arrayspacing"
    return 1
  }
  if {min($rows,$columns) < [lindex [dict keys [dict get $arrayspacing arraycuts]] 0]} {
    # debug "row/columns less than min array spacing"
    return 0
  }
  if {min($rows,$columns) > [lindex [dict keys [dict get $arrayspacing arraycuts]] end]} {
    # debug "row/columns greater than min array spacing"
    return 1
  }
  # debug "default 1"
  return 1
}

proc determine_num_via_columns {via_info constraints} {
  variable upper_width
  variable lower_width
  variable upper_height
  variable lower_height
  variable lower_dir
  variable upper_dir
  variable min_lower_enclosure
  variable max_lower_enclosure
  variable min_upper_enclosure
  variable max_upper_enclosure
  variable cut_width
  variable xcut_pitch
  variable xcut_spacing

  # What are the maximum number of columns that we can fit in this space?
  set i 1
  if {$lower_dir == "hor"} {
    set via_width_lower [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $min_lower_enclosure]
    set via_width_upper [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $max_upper_enclosure]
  } else {
    set via_width_lower [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $max_lower_enclosure]
    set via_width_upper [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $min_upper_enclosure]
  }
  if {[dict exists $constraints cut_pitch]} {set xcut_pitch [ord::microns_to_dbu [dict get $constraints cut_pitch]]}

  while {$via_width_lower <= $lower_width && $via_width_upper <= $upper_width} {
    incr i
    if {$lower_dir == "hor"} {
      set via_width_lower [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $max_lower_enclosure]
      set via_width_upper [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $min_upper_enclosure]
    } else {
      set via_width_lower [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $min_lower_enclosure]
      set via_width_upper [expr $cut_width + $xcut_pitch * ($i - 1) + 2 * $max_upper_enclosure]
    }
  }
  set xcut_spacing [expr $xcut_pitch - $cut_width]
  set columns [expr max(1, $i - 1)]
  # debug "cols $columns W: via_width_lower $via_width_lower >= lower_width $lower_width || via_width_upper $via_width_upper >= upper_width $upper_width"
  if {[dict exists $constraints max_columns]} {
    if {$columns > [dict get $constraints max_columns]} {
      set columns [dict get $constraints max_columns]

      set lower_concave_enclosure [get_concave_spacing_value [dict get $via_info cut layer] [dict get $via_info lower layer]]
      # debug "$lower_concave_enclosure $max_lower_enclosure"
      if {$lower_concave_enclosure > $max_lower_enclosure} {
        set max_lower_enclosure $lower_concave_enclosure
      }
      set upper_concave_enclosure [get_concave_spacing_value [dict get $via_info cut layer] [dict get $via_info upper layer]]
      # debug "$upper_concave_enclosure $max_upper_enclosure"
      if {$upper_concave_enclosure > $max_upper_enclosure} {
        set max_upper_enclosure $upper_concave_enclosure
      }
    }

  }
  # debug "Lower: [dict get $via_info lower layer] $lower_dir"
  # debug "Upper: [dict get $via_info upper layer] $upper_dir"
  if {[get_routing_direction [dict get $via_info upper layer]] == "ver"} {
    if {[dict get $constraints stack_top] != [dict get $via_info upper layer]} {
      # debug "Adjust width of [dict get $via_info upper layer]"
      get_via_enclosure $via_info [expr min($lower_width,$lower_height)] [expr min([expr $cut_width + $xcut_pitch * ($columns - 1)],$upper_height)]
      set upper_width [expr $cut_width + $xcut_pitch * ($columns - 1) + 2 * $min_upper_enclosure]
    }
  }
  if {[get_routing_direction [dict get $via_info lower layer]] == "ver"} {
    if {[dict get $constraints stack_bottom] != [dict get $via_info lower layer]} {
      # debug "Adjust width of [dict get $via_info lower layer]"
      get_via_enclosure $via_info [expr min([expr $cut_width + $xcut_pitch * ($columns - 1)],$lower_height)] [expr min($upper_width,$upper_height)]
      set lower_width [expr $cut_width + $xcut_pitch * ($columns - 1) + 2 * $min_lower_enclosure]
    }
  }
  # debug "cols $columns W: lower $lower_width upper $upper_width"
  set lower_width [get_adjusted_dX [dict get $via_info lower layer] $lower_width]
  set upper_width [get_adjusted_dX [dict get $via_info upper layer] $upper_width]
  # debug "cols $columns W: lower $lower_width upper $upper_width"

  return $columns
}

proc determine_num_via_rows {via_info constraints} {
  variable cut_height
  variable ycut_pitch
  variable ycut_spacing
  variable upper_height
  variable lower_height
  variable lower_width
  variable upper_width
  variable lower_dir
  variable upper_dir
  variable min_lower_enclosure
  variable max_lower_enclosure
  variable min_upper_enclosure
  variable max_upper_enclosure

  # What are the maximum number of rows that we can fit in this space?
  set i 1
  if {$lower_dir == "hor"} {
    set via_height_lower [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $min_lower_enclosure]
    set via_height_upper [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $max_upper_enclosure]
  } else {
    set via_height_lower [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $max_lower_enclosure]
    set via_height_upper [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $min_upper_enclosure]
  }
  if {[dict exists $constraints cut_pitch]} {set ycut_pitch [ord::microns_to_dbu [dict get $constraints cut_pitch]]}
  while {$via_height_lower < $lower_height && $via_height_upper < $upper_height} {
    incr i
    if {$lower_dir == "hor"} {
      set via_height_lower [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $min_lower_enclosure]
      set via_height_upper [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $max_upper_enclosure]
    } else {
      set via_height_lower [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $max_lower_enclosure]
      set via_height_upper [expr $cut_height + $ycut_pitch * ($i - 1) + 2 * $min_upper_enclosure]
    }
  }
  set ycut_spacing [expr $ycut_pitch - $cut_height]
  set rows [expr max(1,$i - 1)]
  # debug "$rows H: $via_height_lower >= $lower_height && $via_height_upper >= $upper_height"
  if {[dict exists $constraints max_rows]} {
    if {$rows > [dict get $constraints max_rows]} {
      set rows [dict get $constraints max_rows]

      set lower_concave_enclosure [get_concave_spacing_value [dict get $via_info cut layer] [dict get $via_info lower layer]]
      # debug "$lower_concave_enclosure $max_lower_enclosure"
      if {$lower_concave_enclosure > $max_lower_enclosure} {
        set max_lower_enclosure $lower_concave_enclosure
      }
      set upper_concave_enclosure [get_concave_spacing_value [dict get $via_info cut layer] [dict get $via_info upper layer]]
      # debug "$upper_concave_enclosure $max_upper_enclosure"
      if {$upper_concave_enclosure > $max_upper_enclosure} {
        set max_upper_enclosure $upper_concave_enclosure
      }

    }
  }
  if {[get_routing_direction [dict get $via_info lower layer]] == "hor"} {
    # debug "[dict get $constraints stack_bottom] != [dict get $via_info lower layer]"
    if {[dict get $constraints stack_bottom] != [dict get $via_info lower layer]} {
      # debug "Adjust height of [dict get $via_info lower layer]"
      get_via_enclosure $via_info [expr min($lower_width,[expr $cut_height + $ycut_pitch * ($rows - 1)])] [expr min($upper_width,$upper_height)]
      set lower_height [expr $cut_height + $ycut_pitch * ($rows - 1) + 2 * $min_lower_enclosure]
      # debug "modify lower_height to $lower_height ($cut_height + $ycut_pitch * ($rows - 1) + 2 * $min_lower_enclosure"
    }
  }
  if {[get_routing_direction [dict get $via_info upper layer]] == "hor"} {
    # debug "[dict get $constraints stack_top] != [dict get $via_info upper layer]"
    if {[dict get $constraints stack_top] != [dict get $via_info upper layer]} {
      # debug "Adjust height of [dict get $via_info upper layer]"
      get_via_enclosure $via_info [expr min($lower_width,$lower_height)] [expr min($upper_width,[expr $cut_height + $ycut_pitch * ($rows - 1)])]
      set upper_height [expr $cut_height + $ycut_pitch * ($rows - 1) + 2 * $min_upper_enclosure]
      # debug "modify upper_height to $upper_height ($cut_height + $ycut_pitch * ($rows - 1) + 2 * $min_upper_enclosure"
    }
  }
  # debug "$rows H: lower $lower_height upper $upper_height"
  set lower_height [get_adjusted_dY [dict get $via_info lower layer] $lower_height]
  set upper_height [get_adjusted_dY [dict get $via_info upper layer] $upper_height]
  # debug "$rows H: lower $lower_height upper $upper_height"

  return $rows
}

proc init_via_width_height {via_info lower_layer width height constraints} {
  variable upper_width
  variable lower_width
  variable upper_height
  variable lower_height
  variable lower_dir
  variable upper_dir
  variable min_lower_enclosure
  variable max_lower_enclosure
  variable min_upper_enclosure
  variable max_upper_enclosure
  variable cut_width
  variable cut_height
  variable xcut_pitch
  variable ycut_pitch
  variable xcut_spacing
  variable ycut_spacing

  set upper_layer [dict get $via_info upper layer]

  set xcut_pitch [lindex [dict get $via_info cut spacing] 0]
  set ycut_pitch [lindex [dict get $via_info cut spacing] 0]

  set cut_width   [lindex [dict get $via_info cut size] 0]
  set cut_height  [lindex [dict get $via_info cut size] 1]

  if {[dict exists $constraints split_cuts $lower_layer]} {
    if {[get_dir $lower_layer] == "hor"} {
      set ycut_pitch [ord::microns_to_dbu [dict get $constraints split_cuts $lower_layer]]
    } else {
      set xcut_pitch [ord::microns_to_dbu [dict get $constraints split_cuts $lower_layer]]
    }
  }

  if {[dict exists $constraints split_cuts $upper_layer]} {
    if {[get_dir $upper_layer] == "hor"} {
      set ycut_pitch [ord::microns_to_dbu [dict get $constraints split_cuts $upper_layer]]
    } else {
      set xcut_pitch [ord::microns_to_dbu [dict get $constraints split_cuts $upper_layer]]
    }
  }

  if {[dict exists $constraints width $lower_layer]} {
    if {[get_dir $lower_layer] == "hor"} {
      set lower_height [ord::microns_to_dbu [dict get $constraints width $lower_layer]]
      set lower_width  [get_adjusted_dX $lower_layer $width]
    } else {
      set lower_width [ord::microns_to_dbu [dict get $constraints width $lower_layer]]
      set lower_height [get_adjusted_dY $lower_layer $height]
    }
  } else {
    # Adjust the width and height values to the next largest allowed value if necessary
    set lower_width  [get_adjusted_dX $lower_layer $width]
    set lower_height [get_adjusted_dY $lower_layer $height]
  }
  if {[dict exists $constraints width $upper_layer]} {
    if {[get_dir $upper_layer] == "hor"} {
      set upper_height [ord::microns_to_dbu [dict get $constraints width $upper_layer]]
      set upper_width  [get_adjusted_dX $upper_layer $width]
    } else {
      set upper_width [ord::microns_to_dbu [dict get $constraints width $upper_layer]]
      set upper_height [get_adjusted_dY $upper_layer $height]
    }
  } else {
    set upper_width  [get_adjusted_dX $upper_layer $width]
    set upper_height [get_adjusted_dY $upper_layer $height]
  }
  # debug "lower (width $lower_width height $lower_height) upper (width $upper_width height $upper_height)"
  # debug "min - \[expr min($lower_width,$lower_height,$upper_width,$upper_height)\]"
}

proc get_enclosure_by_direction {layer xenc yenc max_enclosure min_enclosure} {
  set info {}
  if {$xenc > $max_enclosure && $yenc > $min_enclosure || $xenc > $min_enclosure && $yenc > $max_enclosure} {
    # If the current enclosure values meet the min/max enclosure requirements either way round, then keep
    # the current enclsoure settings
    dict set info xEnclosure $xenc
    dict set info yEnclosure $yenc
  } else {
    # Enforce min/max enclosure rule, with max_enclosure along the preferred direction of the layer.
    if {[get_dir $layer] == "hor"} {
      dict set info xEnclosure [expr max($xenc,$max_enclosure)]
      dict set info yEnclosure [expr max($yenc,$min_enclosure)]
    } else {
      dict set info xEnclosure [expr max($xenc,$min_enclosure)]
      dict set info yEnclosure [expr max($yenc,$max_enclosure)]
    }
  }

  return $info
}

proc via_generate_rule {viarule_name via_info rule_name rows columns constraints} {
  variable xcut_pitch
  variable ycut_pitch
  variable xcut_spacing
  variable ycut_spacing
  variable cut_height
  variable cut_width
  variable upper_width
  variable lower_width
  variable upper_height
  variable lower_height
  variable lower_dir
  variable min_lower_enclosure
  variable max_lower_enclosure
  variable min_upper_enclosure
  variable max_upper_enclosure

  set lower_enc_width  [expr round(($lower_width  - ($cut_width   + $xcut_pitch * ($columns - 1))) / 2)]
  set lower_enc_height [expr round(($lower_height - ($cut_height  + $ycut_pitch * ($rows    - 1))) / 2)]
  set upper_enc_width  [expr round(($upper_width  - ($cut_width   + $xcut_pitch * ($columns - 1))) / 2)]
  set upper_enc_height [expr round(($upper_height - ($cut_height  + $ycut_pitch * ($rows    - 1))) / 2)]

  set lower [get_enclosure_by_direction [dict get $via_info lower layer] $lower_enc_width $lower_enc_height $max_lower_enclosure $min_lower_enclosure]
  set upper [get_enclosure_by_direction [dict get $via_info upper layer] $upper_enc_width $upper_enc_height $max_upper_enclosure $min_upper_enclosure]
  # debug "rule $rule_name"
  # debug "lower: width $lower_width height $lower_height"
  # debug "lower: enc_width $lower_enc_width enc_height $lower_enc_height enclosure_rule $max_lower_enclosure $min_lower_enclosure"
  # debug "lower: enclosure [dict get $lower xEnclosure] [dict get $lower yEnclosure]"
  # debug "upper: enc_width $upper_enc_width enc_height $upper_enc_height enclosure_rule $max_upper_enclosure $min_upper_enclosure"
  # debug "upper: enclosure [dict get $upper xEnclosure] [dict get $upper yEnclosure]"

  return [list [list \
    name $rule_name \
    rule $viarule_name \
    cutsize [dict get $via_info cut size] \
    layers [list [dict get $via_info lower layer] [dict get $via_info cut layer] [dict get $via_info upper layer]] \
    cutspacing [list $xcut_spacing $ycut_spacing] \
    rowcol [list $rows $columns] \
    lower_rect [list [expr -1 * $lower_width / 2] [expr -1 * $lower_height / 2] [expr $lower_width / 2] [expr $lower_height / 2]] \
    upper_rect [list [expr -1 * $upper_width / 2] [expr -1 * $upper_height / 2] [expr $upper_width / 2] [expr $upper_height / 2]] \
    enclosure [list \
      [dict get $lower xEnclosure] \
      [dict get $lower yEnclosure] \
      [dict get $upper xEnclosure] \
      [dict get $upper yEnclosure] \
    ] \
    origin_x 0 origin_y 0
  ]]
}

proc via_generate_array_rule {viarule_name via_info rule_name rows columns} {
  variable xcut_pitch
  variable ycut_pitch
  variable xcut_spacing
  variable ycut_spacing
  variable cut_height
  variable cut_width
  variable upper_width
  variable lower_width
  variable upper_height
  variable lower_height
  variable min_lower_enclosure
  variable max_lower_enclosure
  variable min_upper_enclosure
  variable max_upper_enclosure

  # We need array vias -
  # if the min(rows,columns) > ARRAYCUTS
  #   determine which direction gives best number of CUTs wide using min(ARRAYCUTS)
  #   After adding ARRAYs, is there space for more vias
  #   Add vias to the rule with appropriate origin setting
  # else
  #   add a single via with min(rows,columns) cuts - hor/ver as required


  set spacing_rule [get_arrayspacing_rule [dict get $via_info cut layer]]
  set array_size [expr min($rows, $columns)]

  set lower_enc_width  [expr round(($lower_width  - ($cut_width   + $xcut_pitch * ($columns - 1))) / 2)]
  set lower_enc_height [expr round(($lower_height - ($cut_height  + $ycut_pitch * ($rows    - 1))) / 2)]
  set upper_enc_width  [expr round(($upper_width  - ($cut_width   + $xcut_pitch * ($columns - 1))) / 2)]
  set upper_enc_height [expr round(($upper_height - ($cut_height  + $ycut_pitch * ($rows    - 1))) / 2)]

  if {$array_size > [lindex [dict keys [dict get $spacing_rule arraycuts]] end]} {
    # debug "Multi-viaArrayspacing rule"
    set use_array_size [lindex [dict keys [dict get $spacing_rule arraycuts]] 0]
    foreach other_array_size [lrange [dict keys [dict get $spacing_rule arraycuts]] 1 end] {
      if {$array_size % $use_array_size > $array_size % $other_array_size} {
        set use_array_size $other_array_size
      }
    }
    set num_arrays [expr $array_size / $use_array_size]
    set array_spacing [expr max($xcut_spacing,$ycut_spacing,[dict get $spacing_rule arraycuts $use_array_size spacing])]

    set rule [list \
      rule $viarule_name \
      cutsize [dict get $via_info cut size] \
      layers [list [dict get $via_info lower layer] [dict get $via_info cut layer] [dict get $via_info upper layer]] \
      cutspacing [list $xcut_spacing $ycut_spacing] \
      lower_rect [list [expr -1 * $lower_width / 2] [expr -1 * $lower_height / 2] [expr $lower_width / 2] [expr $lower_height / 2]] \
      upper_rect [list [expr -1 * $upper_width / 2] [expr -1 * $upper_height / 2] [expr $upper_width / 2] [expr $upper_height / 2]] \
      origin_x 0 \
      origin_y 0 \
    ]
    # debug "$rule"
    set rule_list {}
    if {$array_size == $rows} {
      # Split into num_arrays rows of arrays
      set array_min_size [expr [lindex [dict get $via_info cut size] 0] * $use_array_size + [dict get $spacing_rule cutspacing] * ($use_array_size - 1)]
      set total_array_size [expr $array_min_size * $num_arrays + $array_spacing * ($num_arrays - 1)]
      # debug "Split into $num_arrays rows of arrays"

      set lower_enc_height [expr round(($lower_height - ($cut_height  + $ycut_pitch * ($use_array_size - 1))) / 2)]
      set upper_enc_height [expr round(($upper_height - ($cut_height  + $ycut_pitch * ($use_array_size - 1))) / 2)]

      set lower_enc [get_enclosure_by_direction [dict get $via_info lower layer] $lower_enc_width $lower_enc_height $max_lower_enclosure $min_lower_enclosure]
      set upper_enc [get_enclosure_by_direction [dict get $via_info upper layer] $upper_enc_width $upper_enc_height $max_upper_enclosure $min_upper_enclosure]

      dict set rule rowcol [list $use_array_size $columns]
      dict set rule name "[dict get $via_info cut layer]_ARRAY_${use_array_size}X${columns}"
      dict set rule enclosure [list \
        [dict get $lower_enc xEnclosure] \
        [dict get $lower_enc yEnclosure] \
        [dict get $upper_enc xEnclosure] \
        [dict get $upper_enc yEnclosure] \
      ]

      set y [expr $array_min_size / 2 - $total_array_size / 2]
      for {set i 0} {$i < $num_arrays} {incr i} {
        dict set rule origin_y $y
        lappend rule_list $rule
        set y [expr $y + $array_spacing + $array_min_size]
      }
    } else {
      # Split into num_arrays columns of arrays
      set array_min_size [expr [lindex [dict get $via_info cut size] 1] * $use_array_size + [dict get $spacing_rule cutspacing] * ($use_array_size - 1)]
      set total_array_size [expr $array_min_size * $num_arrays + $array_spacing * ($num_arrays - 1)]
      # debug "Split into $num_arrays columns of arrays"

      set lower_enc_width  [expr round(($lower_width  - ($cut_width   + $xcut_pitch * ($use_array_size - 1))) / 2)]
      set upper_enc_width  [expr round(($upper_width  - ($cut_width   + $xcut_pitch * ($use_array_size - 1))) / 2)]

      set lower_enc [get_enclosure_by_direction [dict get $via_info lower layer] $lower_enc_width $lower_enc_height $max_lower_enclosure $min_lower_enclosure]
      set upper_enc [get_enclosure_by_direction [dict get $via_info upper layer] $upper_enc_width $upper_enc_height $max_upper_enclosure $min_upper_enclosure]

      dict set rule rowcol [list $rows $use_array_size]
      dict set rule name "[dict get $via_info cut layer]_ARRAY_${rows}X${use_array_size}"
      dict set rule enclosure [list \
        [dict get $lower_enc xEnclosure] \
        [dict get $lower_enc yEnclosure] \
        [dict get $upper_enc xEnclosure] \
        [dict get $upper_enc yEnclosure] \
      ]

      set x [expr $array_min_size / 2 - $total_array_size / 2]
      for {set i 0} {$i < $num_arrays} {incr i} {
        dict set rule origin_x $x
        lappend rule_list $rule
        set x [expr $x + $array_spacing + $array_min_size]
      }
    }
  } else {
    # debug "Arrayspacing rule"
    set lower_enc [get_enclosure_by_direction [dict get $via_info lower layer] $lower_enc_width $lower_enc_height $max_lower_enclosure $min_lower_enclosure]
    set upper_enc [get_enclosure_by_direction [dict get $via_info upper layer] $upper_enc_width $upper_enc_height $max_upper_enclosure $min_upper_enclosure]

    set rule [list \
      name $rule_name \
      rule $viarule_name \
      cutsize [dict get $via_info cut size] \
      layers [list [dict get $via_info lower layer] [dict get $via_info cut layer] [dict get $via_info upper layer]] \
      cutspacing [list $xcut_spacing $ycut_spacing] \
      rowcol [list $rows $columns] \
      enclosure [list \
        [dict get $lower_enc xEnclosure] \
        [dict get $lower_enc yEnclosure] \
        [dict get $upper_enc xEnclosure] \
        [dict get $upper_enc yEnclosure] \
      ] \
      origin_x 0 \
      origin_y 0 \
    ]
    set rule_list [list $rule]
  }

  return $rule_list
}

proc via_split_cuts_rule {rule_name via_info rows columns constraints} {
  variable tech
  variable min_lower_enclosure
  variable max_lower_enclosure
  variable min_upper_enclosure
  variable max_upper_enclosure
  variable cut_width
  variable cut_height
  variable xcut_pitch
  variable ycut_pitch
  variable xcut_spacing
  variable ycut_spacing

  set lower_rects {}
  set cut_rects   {}
  set upper_rects {}

  set lower [dict get $via_info lower layer]
  set upper [dict get $via_info upper layer]
  # debug $via_info
  # debug "lower $lower upper $upper"

  set rule {}
  set rule [list \
    rule $rule_name \
    cutsize [dict get $via_info cut size] \
    layers [list $lower [dict get $via_info cut layer] $upper] \
    cutspacing [list $xcut_spacing $ycut_spacing] \
    rowcol [list 1 1] \
  ]

  # Enclosure was calculated from full width of intersection - need to recalculate for min cut size.
  get_via_enclosure $via_info 0 0

  # Area is stored in real units, adjust to def_units
  set lower_area [ord::microns_to_dbu [ord::microns_to_dbu [[find_layer $lower] getArea]]]
  set upper_area [ord::microns_to_dbu [ord::microns_to_dbu [[find_layer $upper] getArea]]]

  if {[get_dir $lower] == "hor"} {
    set lower_height [expr $cut_height + $min_lower_enclosure]
    set lower_width  [expr $cut_width  + $max_lower_enclosure]
    set upper_height [expr $cut_height + $max_upper_enclosure]
    set upper_width  [expr $cut_width  + $min_upper_enclosure]

    if {[dict exists $constraints split_cuts $lower]} {
      set lower_width  [expr $lower_area / $lower_height]
      if {$lower_width % 2 == 1} {incr lower_width}
      set max_lower_enclosure [expr max(($lower_width - $cut_width) / 2, $max_lower_enclosure)]
    }

    if {[dict exists $constraints split_cuts $upper]} {
      set upper_height [expr $upper_area / $upper_width]
      if {$upper_height % 2 == 1} {incr upper_height}
      set max_upper_enclosure [expr max(($upper_height - $cut_height) / 2, $max_upper_enclosure)]
    }

    set width [expr $max_lower_enclosure * 2 + $cut_width]
    set height [expr $max_upper_enclosure * 2 + $cut_width]

    dict set rule name [get_viarule_name $lower $width $height]
    dict set rule enclosure [list $max_lower_enclosure $min_lower_enclosure $min_upper_enclosure $max_upper_enclosure]
  } else {
    set lower_height [expr $cut_height + $max_lower_enclosure]
    set lower_width  [expr $cut_width  + $min_lower_enclosure]
    set upper_height [expr $cut_height + $min_upper_enclosure]
    set upper_width  [expr $cut_width  + $max_upper_enclosure]

    if {[dict exists $constraints split_cuts $lower]} {
      set lower_width  [expr $cut_width + $min_lower_enclosure]
      set lower_height [expr $cut_width + $max_lower_enclosure]
      set min_lower_length [expr $lower_area / $lower_width]
      if {$min_lower_length % 2 == 1} {incr min_lower_length}
      set max_lower_enclosure [expr max(($min_lower_length - $cut_width) / 2, $max_lower_enclosure)]
    }

    if {[dict exists $constraints split_cuts $upper]} {
      set upper_width  [expr $cut_height + $max_upper_enclosure]
      set upper_height [expr $cut_height + $min_upper_enclosure]
      set min_upper_length [expr $upper_area / $upper_height]
      if {$min_upper_length % 2 == 1} {incr min_upper_length}
      set max_upper_enclosure [expr max(($min_upper_length - $cut_height) / 2, $max_upper_enclosure)]
    }

    set width [expr $max_upper_enclosure * 2 + $cut_width]
    set height [expr $max_lower_enclosure * 2 + $cut_width]

    dict set rule name [get_viarule_name $lower $width $height]
    dict set rule enclosure [list $min_lower_enclosure $max_lower_enclosure $max_upper_enclosure $min_upper_enclosure]
  }
  dict set rule lower_rect [list [expr -1 * $lower_width / 2] [expr -1 * $lower_height / 2] [expr $lower_width / 2] [expr $lower_height / 2]]
  dict set rule upper_rect [list [expr -1 * $upper_width / 2] [expr -1 * $upper_height / 2] [expr $upper_width / 2] [expr $upper_height / 2]]
  # debug "min_lower_enclosure $min_lower_enclosure"
  # debug "lower $lower upper $upper enclosure [dict get $rule enclosure]"

  for {set i 0} {$i < $rows} {incr i} {
    for {set j 0} {$j < $columns} {incr j} {
      set centre_x [expr round(($j - (($columns - 1) / 2.0)) * $xcut_pitch)]
      set centre_y [expr round(($i - (($rows - 1)    / 2.0)) * $ycut_pitch)]

      dict set rule origin_x $centre_x
      dict set rule origin_y $centre_y
      lappend rule_list $rule
    }
  }
  # debug "split into [llength $rule_list] vias"
  return $rule_list
}

# viarule structure:
# {
#    name <via_name>
#    rule <via_rule_name>
#    cutsize {<cut_size>}
#    layers {<lower> <cut> <upper>}
#    cutspacing {<x_spacing> <y_spacing>}
#    rowcol {<rows> <columns>}
#    origin_x <x_location>
#    origin_y <y_location>
#    enclosure {<x_lower_enclosure> <y_lower_enclosure> <x_upper_enclosure> <y_upper_enclosure>}
#    lower_rect {<llx> <lly> <urx> <ury>}
#  }

# Given the via rule expressed in via_info, what is the via with the largest cut area that we can make
# Try using a via generate rule
proc get_via_option {viarule_name via_info lower width height constraints} {
  variable upper_width
  variable lower_width
  variable upper_height
  variable lower_height
  variable lower_dir
  variable upper_dir
  variable min_lower_enclosure
  variable max_lower_enclosure
  variable min_upper_enclosure
  variable max_upper_enclosure
  variable default_cutclass
  variable grid_data
  variable via_location

  set upper [dict get $via_info upper layer]

  # debug "{$lower $width $height}"

  set lower_dir [get_dir $lower]
  set upper_dir [get_dir $upper]

  init_via_width_height $via_info $lower $width $height $constraints
  # debug "lower: $lower, width: $width, height: $height, lower_width: $lower_width, lower_height: $lower_height"
  get_via_enclosure $via_info [expr min($lower_width,$lower_height)] [expr min($upper_width,$upper_height)]

  # debug "split cuts? [dict exists $constraints split_cuts]"
  # debug "lower $lower upper $upper"
  # debug [dict get $via_info cut layer]

  # Determines the maximum number of rows and columns that can fit into this width/height
  set columns [determine_num_via_columns $via_info $constraints]
  set rows    [determine_num_via_rows    $via_info $constraints]

  # debug "columns: $columns, rows: $rows"
  # debug "lower_width $lower_width lower_height: $lower_height, min_lower_enclosure $min_lower_enclosure"
  # debug "upper_width $upper_width upper_height: $upper_height, min_upper_enclosure $min_upper_enclosure"

  if {[dict exists $constraints split_cuts] && ([lsearch -exact [dict get $constraints split_cuts] $lower] > -1 || [lsearch -exact [dict get $constraints split_cuts] $upper] > -1)} {
    # debug "via_split_cuts_rule"
    set rules [via_split_cuts_rule $viarule_name $via_info $rows $columns $constraints]
  } elseif {[use_arrayspacing [dict get $via_info cut layer] $rows $columns]} {
    # debug "via_generate_array_rule"
    set rules [via_generate_array_rule $viarule_name $via_info [get_viarule_name $lower $width $height] $rows $columns]
  } else {
    # debug "via_generate_rule"
    set rules [via_generate_rule $viarule_name $via_info [get_viarule_name $lower $width $height] $rows $columns $constraints]
  }

  # Check minimum_cuts
  set checked_rules {}
  foreach via_rule $rules {
    # debug "$via_rule"
    set num_cuts [expr [lindex [dict get $via_rule rowcol] 0] * [lindex [dict get $via_rule rowcol] 1]]
    if {[dict exists $default_cutclass [lindex [dict get $via_rule layers] 1]]} {
      set cut_class [dict get $default_cutclass [lindex [dict get $via_rule layers] 1]]
    } else {
      set cut_class "NONE"
    }
    set lower_layer [lindex [dict get $via_rule layers] 0]
    if {[dict exists $constraints stack_bottom]} {
      if {[dict exists $grid_data straps $lower_layer] || [dict exists $grid_data rails $lower_layer]} {
        set lower_width [get_grid_wire_width $lower_layer]
      } else {
        set lower_rect [dict get $via_rule lower_rect]
        set lower_width [expr min(([lindex $lower_rect 2] - [lindex $lower_rect 0]), ([lindex $lower_rect 3] - [lindex $lower_rect 1]))]
      }
    } else {
      set lower_rect [dict get $via_rule lower_rect]
      set lower_width [expr min(([lindex $lower_rect 2] - [lindex $lower_rect 0]), ([lindex $lower_rect 3] - [lindex $lower_rect 1]))]
    }
    set min_cut_rule [get_minimumcuts $lower_layer $lower_width fromabove $cut_class]
    if {$num_cuts < $min_cut_rule} {
      utl::warn PDN 9038 "Illegal via: number of cuts ($num_cuts), does not meet minimum cut rule ($min_cut_rule) for $lower_layer to $cut_class with width [ord::dbu_to_microns $lower_width]."
      dict set via_rule illegal 1
    } else {
      # debug "Legal number of cuts ($num_cuts) meets minimum cut rule ($min_cut_rule) for $lower_layer, $lower_width, $cut_class"
    }

    set upper_layer [lindex [dict get $via_rule layers] 2]
    if {[dict exists $constraints stack_top]} {
      if {[dict exists $grid_data straps $upper_layer width] || [dict exists $grid_data rails $upper_layer width]} {
        set upper_width [get_grid_wire_width $upper_layer]
      } else {
        set upper_rect [dict get $via_rule upper_rect]
        set upper_width [expr min(([lindex $upper_rect 2] - [lindex $upper_rect 0]), ([lindex $upper_rect 3] - [lindex $upper_rect 1]))]
      }
    } else {
      set upper_rect [dict get $via_rule upper_rect]
      set upper_width [expr min(([lindex $upper_rect 2] - [lindex $upper_rect 0]), ([lindex $upper_rect 3] - [lindex $upper_rect 1]))]
    }
    set min_cut_rule [get_minimumcuts $upper_layer $upper_width frombelow $cut_class]

    if {$num_cuts < $min_cut_rule} {
      utl::warn PDN 9039 "Illegal via: number of cuts ($num_cuts), does not meet minimum cut rule ($min_cut_rule) for $upper_layer to $cut_class with width [ord::dbu_to_microns $upper_width]."
      dict set via_rule illegal 1
    } else {
      # debug "Legal number of cuts ($num_cuts) meets minimum cut rule ($min_cut_rule) for $upper_layer, $upper_width $cut_class"
    }
    if {[dict exists $via_rule illegal]} {
      utl::warn PDN 9036 "Attempt to add illegal via at : ([ord::dbu_to_microns [lindex $via_location 0]] [ord::dbu_to_microns [lindex $via_location 1]]), via will not be added."
    }
    lappend checked_rules $via_rule
  }

  return $checked_rules
}

proc get_viarule_name {lower width height} {
  set rules [select_via_info $lower]
  if {[llength $rules] > 0} {
    set first_key [lindex [dict keys $rules] 0]
    #if {![dict exists $rules $first_key cut layer]} {
    #  debug "$lower $width $height"
    #  debug "$rules"
    #  debug "$first_key"
    #}
    set cut_layer [dict get $rules $first_key cut layer]
  } else {
    set cut_layer $lower
  }

  return ${cut_layer}_${width}x${height}
}

proc get_cut_area {rule} {
  set area 0
  foreach via $rule {
    set area [expr [lindex [dict get $via rowcol] 0] * [lindex [dict get $via rowcol] 0] * [lindex [dict get $via cutsize] 0] * [lindex [dict get $via cutsize] 1]]
  }
  return $area
}

proc select_rule {rule1 rule2} {
  if {[get_cut_area $rule2] > [get_cut_area $rule1]} {
    return $rule2
  }
  return $rule1
}

proc connection_specifies_fixed_via {constraints lower} {
  if {[dict exists $constraints use_fixed_via]} {
    return [dict exists $constraints use_fixed_via $lower]
  }
  return 0
}

proc get_via {lower width height constraints} {
  # First cur will assume that all crossing points (x y) are on grid for both lower and upper layers
  # TODO: Refine the algorithm to cope with offgrid intersection points
  variable physical_viarules

  set rule_name [get_viarule_name $lower $width $height]

  if {![dict exists $physical_viarules $rule_name]} {
    set selected_rule {}
    # debug "$constraints"
    if {[connection_specifies_fixed_via $constraints $lower]} {
      # debug "Using fixed_via for $rule_name"
      set via_name [dict get $constraints use_fixed_via $lower]
      dict set physical_viarules $rule_name [list [list name $via_name fixed $via_name origin_x 0 origin_y 0 layers [list $lower "cut" "upper"]]]
    } else {
      dict for {name rule} [select_via_info $lower] {
        set result [get_via_option $name $rule $lower $width $height $constraints]
        if {$selected_rule == {}} {
          set selected_rule $result
        } else {
          # Choose the best between selected rule and current result, the winner becomes the new selected rule
          set selected_rule [select_rule $selected_rule $result]
        }
      }
      dict set physical_viarules $rule_name $selected_rule
      # debug "Via [dict size $physical_viarules]: $rule_name"
    }
  }

  return $rule_name
}

proc instantiate_via {physical_via_name x y constraints} {
  variable physical_viarules
  variable block
  variable layers

  set via_insts {}

  foreach via [dict get $physical_viarules $physical_via_name] {
    # debug "via x $x y $y $via"

    # Dont instantiate illegal vias
    if {[dict exists $via illegal]} {continue}

    set x_location [expr $x + [dict get $via origin_x]]
    set y_location [expr $y + [dict get $via origin_y]]

    set lower_layer_name [lindex [dict get $via layers] 0]
    set upper_layer_name [lindex [dict get $via layers] 2]

    if {[dict exists $constraints ongrid]} {
      if {[lsearch -exact [dict get $constraints ongrid] $lower_layer_name] > -1} {
        if {[get_dir $lower_layer_name] == "hor"} {
          set y_pitch [dict get $layers $lower_layer_name pitch]
          set y_offset [dict get $layers $lower_layer_name offsetY]

          set y_location [expr ($y - $y_offset + $y_pitch / 2) / $y_pitch * $y_pitch + $y_offset + [dict get $via origin_y]]
        } else {
          set x_pitch [dict get $layers $lower_layer_name pitch]
          set x_offset [dict get $layers $lower_layer_name offsetX]

          set x_location [expr ($x - $x_offset + $x_pitch / 2) / $x_pitch * $x_pitch + $x_offset + [dict get $via origin_x]]
        }
      }
      if {[lsearch -exact [dict get $constraints ongrid] $upper_layer_name] > -1} {
        if {[get_dir $lower_layer_name] == "hor"} {
          set x_pitch [dict get $layers $upper_layer_name pitch]
          set x_offset [dict get $layers $upper_layer_name offsetX]

          set x_location [expr ($x - $x_offset + $x_pitch / 2) / $x_pitch * $x_pitch + $x_offset + [dict get $via origin_x]]
        } else {
          set y_pitch [dict get $layers $upper_layer_name pitch]
          set y_offset [dict get $layers $upper_layer_name offsetY]

          set y_location [expr ($y - $y_offset + $y_pitch / 2) / $y_pitch * $y_pitch + $y_offset + [dict get $via origin_y]]
        }
      }
    }
    # debug "x: $x -> $x_location"
    # debug "y: $y -> $y_location"

    dict set via x $x_location
    dict set via y $y_location

    lappend via_insts $via
  }
  return $via_insts
}

proc generate_vias {layer1 layer2 intersections connection} {
  variable logical_viarules
  variable metal_layers
  variable via_location
  variable tech

  set constraints {}
  if {[dict exists $connection constraints]} {
    set constraints [dict get $connection constraints]
  }
  if {[dict exists $connection fixed_vias]} {
    foreach via_name [dict get $connection fixed_vias] {
      if {[set via [$tech findVia $via_name]] != "NULL"} {
        set lower_layer_name [[$via getBottomLayer] getName]
        dict set constraints use_fixed_via $lower_layer_name $via_name
      } else {
        utl::warn PDN 9063 "Via $via_name specified in the grid specification does not exist in this technology."
      }
    }
  }

  # debug "    Constraints: $constraints"
  set vias {}
  set layer1_name $layer1
  set layer2_name $layer2
  regexp {(.*)_PIN_(hor|ver)} $layer1 - layer1_name layer1_direction

  set i1 [lsearch -exact $metal_layers $layer1_name]
  set i2 [lsearch -exact $metal_layers $layer2_name]
  if {$i1 == -1} {utl::error PDN 9022 "Cannot find lower metal layer $layer1."}
  if {$i2 == -1} {utl::error PDN 9023 "Cannot find upper metal layer $layer2."}

  # For each layer between l1 and l2, add vias at the intersection
  # debug "  # Intersections [llength $intersections]"
  set count 0
  foreach intersection $intersections {
    if {![dict exists $logical_viarules [dict get $intersection rule]]} {
      utl::error PDN 9024 "Missing logical viarule [dict get $intersection rule].\nAvailable logical viarules [dict keys $logical_viarules]."
    }
    set logical_rule [dict get $logical_viarules [dict get $intersection rule]]

    set x [dict get $intersection x]
    set y [dict get $intersection y]
    set width  [dict get $logical_rule width]
    set height  [dict get $logical_rule height]
    set via_location [list $x $y]

    set connection_layers [lrange $metal_layers $i1 [expr $i2 - 1]]
    # debug "  # Connection layers: [llength $connection_layers]"
    # debug "  Connection layers: $connection_layers"
    dict set constraints stack_top $layer2_name
    dict set constraints stack_bottom $layer1_name
    foreach lay $connection_layers {
      set via_name [get_via $lay $width $height $constraints]
      foreach via [instantiate_via $via_name $x $y $constraints] {
        lappend vias $via
      }
    }

    incr count
    #if {$count % 1000 == 0} {
    #  debug "  # $count / [llength $intersections]"
    #}
  }

  return $vias
}

proc get_layers_from_to {from to} {
  variable metal_layers

  set layers {}
  for {set i [lsearch -exact $metal_layers $from]} {$i <= [lsearch -exact $metal_layers $to]} {incr i} {
    lappend layers [lindex $metal_layers $i]
  }
  return $layers
}

proc get_grid_channel_layers {} {
  variable grid_data

  set channel_layers {}
  if {[dict exists $grid_data rails]} {
    lappend channel_layers [lindex [dict keys [dict get $grid_data rails]] end]
  }
  foreach layer_name [dict keys [dict get $grid_data straps]] {
    lappend channel_layers $layer_name
  }

  return $channel_layers
}

proc get_grid_channel_spacing {layer_name parallel_length} {
  variable grid_data

  if {[dict exists $grid_data straps $layer_name channel_spacing]} {
    return [ord::microns_to_dbu [dict get $grid_data straps $layer_name channel_spacing]]
  } elseif {[dict exists $grid_data straps $layer_name] && [dict exists $grid_data template names]} {
    set template_name [lindex [dict get $grid_data template names] 0]
    if {[dict exists $grid_data straps $layer_name $template_name channel_spacing]} {
      return [expr round([dict get $grid_data straps $layer_name $template_name channel_spacing]]
    }
  } else {
    set layer [[ord::get_db_tech] findLayer $layer_name]
    if {$layer == "NULL"}  {
      utl::error PDN 9168 "Layer $layer_name does not exist"
    }
    set layer_width [get_grid_wire_width $layer_name]
    if {[$layer hasTwoWidthsSpacingRules]} {
      set num_widths [$layer getTwoWidthsSpacingTableNumWidths]
      set current_width 0
      set prl_rule -1
      for {set rule 0} {$rule < $num_widths} {incr rule} {
        set width [$layer getTwoWidthsSpacingTableWidth $rule]
        if {$width == $current_width && $prl_rule != -1} {
          continue
        } else {
          set current_width $width
          if {[$layer getTwoWidthsSpacingTableHasPRL $rule] == 0} {
            set non_prl_rule $rule
            set prl_rule -1
          } else {
            if {$parallel_length > [$layer getTwoWidthsSpacingTablePRL $rule]} {
              set prl_rule $rule
            }
          }
        }
        if {$layer_width < [$layer getTwoWidthsSpacingTableWidth $rule]} {
          if {$prl_rule == 0} {
            set use_rule $non_prl_rule
          } else {
            set use_rule $prl_rule
          }
          break
        }
      }

      set spacing [$layer getTwoWidthsSpacingTableEntry $use_rule $use_rule]
      # debug "Two widths spacing: layer: $layer_name, rule: $use_rule, spacing: $spacing"
    } elseif {[$layer hasV55SpacingRules]} {
      set layer_width [get_grid_wire_width $layer_name]
      set spacing [$layer findV55Spacing $layer_width $parallel_length]
    } else {
      set spacing [$layer getSpacing]
    }
    # Can't store value, since it depends on channel height
    return $spacing
  }

  utl::error PDN 9052 "Unable to get channel_spacing setting for layer $layer_name."
}

proc get_grid_wire_width {layer_name} {
  variable grid_data
  variable default_grid_data
  variable design_data

  if {[info exists grid_data]} {
    if {[dict exists $grid_data rails $layer_name width]} {
      set width [dict get $grid_data rails $layer_name width]
      return $width
    } elseif {[dict exists $grid_data straps $layer_name width]} {
      set width [dict get $grid_data straps $layer_name width]
      return $width
    } elseif {[dict exists $grid_data straps $layer_name] && [dict exists $grid_data template names]} {
      set template_name [lindex [dict get $grid_data template names] 0]
      set width [dict get $grid_data straps $layer_name $template_name width]
      return $width
    } elseif {[dict exists $grid_data core_ring $layer_name width]} {
      set width [dict get $grid_data core_ring $layer_name width]
      return $width
    }
  }

  if {[info exists default_grid_data]} {
    if {[dict exists $default_grid_data rails $layer_name width]} {
      set width [dict get $default_grid_data rails $layer_name width]
      return $width
    } elseif {[dict exists $default_grid_data straps $layer_name width]} {
      set width [dict get $default_grid_data straps $layer_name width]
      return $width
    } elseif {[dict exists $default_grid_data straps $layer_name] && [dict exists $default_grid_data template names]} {
      set template_name [lindex [dict get $default_grid_data template names] 0]
      set width [dict get $default_grid_data straps $layer_name $template_name width]
      return $width
    }
  }
  utl::error PDN 9044 "No width information found for $layer_name."
}

proc get_grid_wire_pitch {layer_name} {
  variable grid_data
  variable default_grid_data
  variable design_data

  if {[dict exists $grid_data rails $layer_name pitch]} {
    set pitch [dict get $grid_data rails $layer_name pitch]
  } elseif {[dict exists $grid_data straps $layer_name pitch]} {
    set pitch [dict get $grid_data straps $layer_name pitch]
  } elseif {[dict exists $grid_data straps $layer_name] && [dict exists $grid_data template names]} {
    set template_name [lindex [dict get $grid_data template names] 0]
    set pitch [dict get $grid_data straps $layer_name $template_name pitch]
  } elseif {[dict exists $default_grid_data straps $layer_name pitch]} {
    set pitch [dict get $default_grid_data straps $layer_name pitch]
  } elseif {[dict exists $default_grid_data straps $layer_name] && [dict exists $default_grid_data template names]} {
    set template_name [lindex [dict get $default_grid_data template names] 0]
    set pitch [dict get $default_grid_data straps $layer_name $template_name pitch]
  } else {
    utl::error PDN 9045 "No pitch information found for $layer_name."
  }

  return $pitch
}


proc get_core_domain_name {} {
  variable design_data

  return [dict get $design_data core_domain]
}

## Proc to generate via locations, both for a normal via and stacked via
proc generate_via_stacks {l1 l2 net connection} {
  variable logical_viarules
  variable stripe_locs
  variable grid_data

  set area [dict get $grid_data area]
  # debug "From $l1 to $l2"

  set core_domain_name [get_core_domain_name]
  # debug "core_domain: [get_core_domain_name] ring area: [dict get $grid_data core_ring_area [get_core_domain_name]]"
  if {[dict exists $grid_data core_ring_area $core_domain_name combined]} {
    set grid_area [dict get $grid_data core_ring_area $core_domain_name combined]
    set factor [expr max([lindex $area 2] - [lindex $area 0], [lindex $area 3] - [lindex $area 1]) * 2]
    set grid_area [odb::shrinkSet [odb::bloatSet $grid_area $factor] $factor]
    # debug "Old area ($area)"
    set bbox [lindex [odb::getRectangles $grid_area] 0]
    set area [list {*}[$bbox ll] {*}[$bbox ur]]
    # debug "Recalculated area to be ($area)"
  }

  #this variable contains locations of intersecting points of two orthogonal metal layers, between which via needs to be inserted
  #for every intersection. Here l1 and l2 are layer names, and i1 and i2 and their indices
  set intersections ""
  #check if layer pair is orthogonal, case 1
  set layer1 $l1
  regexp {(.*)_PIN_(hor|ver)} $l1 - layer1 direction

  set layer2 $l2

  set ignore_count 0
  if {[array names stripe_locs "$l1,$net"] == ""} {
    utl::warn PDN 9002 "No shapes on layer $l1 for $net."
    return {}
  }
  if {[array names stripe_locs "$l2,$net"] == ""} {
    utl::warn PDN 9003 "No shapes on layer $l2 for $net."
    return {}
  }
  set intersection [odb::andSet [odb::andSet $stripe_locs($l1,$net) $stripe_locs($l2,$net)] [odb::newSetFromRect {*}$area]]

  # debug "Detected [llength [::odb::getPolygons $intersection]] intersections of $l1 and $l2"

  foreach shape [::odb::getPolygons $intersection] {
    set points [::odb::getPoints $shape]
    if {[llength $points] != 4} {
        utl::warn PDN 9004 "Unexpected number of points in connection shape ($l1,$l2 $net [llength $points])."
        set str "    "
        foreach point $points {set str "$str ([ord::dbu_to_microns [$point getX]] [ord::dbu_to_microns [$point getY]]) "}
        utl::warn PDN 9005 $str
        continue
    }
    set xMin [expr min([[lindex $points 0] getX], [[lindex $points 1] getX], [[lindex $points 2] getX], [[lindex $points 3] getX])]
    set xMax [expr max([[lindex $points 0] getX], [[lindex $points 1] getX], [[lindex $points 2] getX], [[lindex $points 3] getX])]
    set yMin [expr min([[lindex $points 0] getY], [[lindex $points 1] getY], [[lindex $points 2] getY], [[lindex $points 3] getY])]
    set yMax [expr max([[lindex $points 0] getY], [[lindex $points 1] getY], [[lindex $points 2] getY], [[lindex $points 3] getY])]

    set width [expr $xMax - $xMin]
    set height [expr $yMax - $yMin]

    # Ensure that the intersections are not partial
    if {![regexp {(.*)_PIN_(hor|ver)} $l1]} {
      if {[get_dir $layer1] == "hor"} {
        if {$height < [get_grid_wire_width $layer1]} {
          # If the intersection doesnt cover the whole width of the bottom level wire, then ignore
          utl::warn PDN 9040 "No via added at ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin] [ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax]) because the full height of $layer1 ([ord::dbu_to_microns [get_grid_wire_width $layer1]]) is not covered by the overlap."
          continue
        }
      } else {
        if {$width < [get_grid_wire_width $layer1]} {
          # If the intersection doesnt cover the whole width of the bottom level wire, then ignore
          utl::warn PDN 9041 "No via added at ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin] [ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax]) because the full width of $layer1 ([ord::dbu_to_microns [get_grid_wire_width $layer1]]) is not covered by the overlap."
          continue
        }
      }
    }
    if {[get_dir $layer2] == "hor"} {
      if {$height < [get_grid_wire_width $layer2]} {
        # If the intersection doesnt cover the whole width of the top level wire, then ignore
        utl::warn PDN 9042 "No via added at ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin] [ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax]) because the full height of $layer2 ([ord::dbu_to_microns [get_grid_wire_width $layer2]]) is not covered by the overlap."
        continue
      }
    } else {
      if {$width < [get_grid_wire_width $layer2]} {
        # If the intersection doesnt cover the whole width of the top level wire, then ignore
        utl::warn PDN 9043 "No via added at ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin] [ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax]) because the full width of $layer2 ([ord::dbu_to_microns [get_grid_wire_width $layer2]]) is not covered by the overlap."
        continue
      }
    }

    set rule_name ${l1}${layer2}_${width}x${height}
    if {![dict exists $logical_viarules $rule_name]} {
      dict set logical_viarules $rule_name [list lower $l1 upper $layer2 width $width height $height]
    }
    lappend intersections "rule $rule_name x [expr ($xMax + $xMin) / 2] y [expr ($yMax + $yMin) / 2]"
  }

  # debug "Added [llength $intersections] intersections"

  return [generate_vias $l1 $l2 $intersections $connection]
}

proc add_stripe {layer type polygon_set} {
  variable stripes
  # debug "start"
  lappend stripes($layer,$type) $polygon_set
  # debug "end"
}

proc merge_stripes {} {
  variable stripes
  variable stripe_locs

  foreach stripe_set [array names stripes] {
    # debug "$stripe_set [llength $stripes($stripe_set)]"
    if {[llength $stripes($stripe_set)] > 0} {
      set merged_stripes [shapes_to_polygonSet $stripes($stripe_set)]
      if {[array names stripe_locs $stripe_set] != ""} {
        # debug "$stripe_locs($stripe_set)"
        set stripe_locs($stripe_set) [odb::orSet $stripe_locs($stripe_set) $merged_stripes]
      } else {
        set stripe_locs($stripe_set) $merged_stripes
      }
    }
    set stripes($stripe_set) {}
  }
}

proc get_core_ring_vertical_layer_name {} {
  variable grid_data

  if {![dict exists $grid_data core_ring]} {
    return ""
  }

  foreach layer_name [dict keys [dict get $grid_data core_ring]] {
    if {[get_dir $layer_name] == "ver"} {
      return $layer_name
    }
  }

  return ""
}

proc is_extend_to_core_ring {layer_name} {
  variable grid_data

  if {![dict exists $grid_data rails $layer_name extend_to_core_ring]} {
    # debug "No extend flag found"
    return 0
  }
  if {![dict get $grid_data rails $layer_name extend_to_core_ring]} {
    # debug "extend flag false"
    return 0
  }
  if {[get_core_ring_vertical_layer_name] == ""} {
    # debug "No vertical ring layer"
    return 0
  }
  return 1
}

# proc to generate follow pin layers or standard cell rails
proc generate_lower_metal_followpin_rails {} {
  variable block
  variable grid_data
  variable design_data

  set stdcell_area [get_extent [get_stdcell_area]]
  set stdcell_min_x [lindex $stdcell_area 0]
  set stdcell_max_x [lindex $stdcell_area 2]

  if {[set ring_vertical_layer [get_core_ring_vertical_layer_name]] != ""} {
    # debug "Ring vertical layer: $ring_vertical_layer"
    # debug "Grid_data: $grid_data"
    if {[dict exists $grid_data core_ring $ring_vertical_layer pad_offset]} {
      set pad_area [find_pad_offset_area]
      set offset [expr [dict get $grid_data core_ring $ring_vertical_layer pad_offset]]
      set ring_adjustment [expr $stdcell_min_x - ([lindex $pad_area 0] + $offset)]
    }
    if {[dict exists $grid_data core_ring $ring_vertical_layer core_offset]} {
      set ring_adjustment [expr \
        [dict get $grid_data core_ring $ring_vertical_layer core_offset] + \
        [dict get $grid_data core_ring $ring_vertical_layer spacing] + \
        3 * [dict get $grid_data core_ring $ring_vertical_layer width] / 2 \
      ]
    }
  }

  foreach row [$block getRows] {
    set orient [$row getOrient]
    set box [$row getBBox]
    switch -exact $orient {
      R0 {
        set vdd_y [$box yMax]
        set vss_y [$box yMin]
      }
      MX {
        set vdd_y [$box yMin]
        set vss_y [$box yMax]
      }
      default {
        utl::error PDN 9025 "Unexpected row orientation $orient for row [$row getName]."
      }
    }

    foreach lay [get_rails_layers] {
      set xMin [$box xMin]
      set xMax [$box xMax]
      if {[is_extend_to_core_ring $lay]} {
        # debug "Extending to core_ring - adjustment $ring_adjustment ($xMin/$xMax) ($stdcell_min_x/$stdcell_max_x)"
        set voltage_domain [get_voltage_domain $xMin [$box yMin] $xMax [$box yMax]]
        if {$voltage_domain == [dict get $design_data core_domain]} {
          if {$xMin == $stdcell_min_x} {
            set xMin [expr $xMin - $ring_adjustment]
          }
          if {$xMax == $stdcell_max_x} {
            set xMax [expr $xMax + $ring_adjustment]
          }
        } else {
          #Create lower metal followpin rails for voltage domains where the starting positions are not stdcell_min_x
          set core_power [get_voltage_domain_power [dict get $design_data core_domain]]
          set core_ground [get_voltage_domain_ground [dict get $design_data core_domain]]
          set domain_power [get_voltage_domain_power $voltage_domain]
          set domain_ground [get_voltage_domain_ground $voltage_domain]

          set first_rect [lindex [[$block findRegion $voltage_domain] getBoundaries] 0]
          set domain_xMin [$first_rect xMin]
          set domain_xMax [$first_rect xMax]

          if {$xMin == $domain_xMin} {
            set xMin [expr $xMin - $ring_adjustment]
          }
          if {$xMax == $domain_xMax} {
            set xMax [expr $xMax + $ring_adjustment]
          }
        }
        # debug "Extended  to core_ring - adjustment $ring_adjustment ($xMin/$xMax)"
      }
      set width [dict get $grid_data rails $lay width]
      # debug "VDD: $xMin [expr $vdd_y - $width / 2] $xMax [expr $vdd_y + $width / 2]"
      set vdd_box [::odb::newSetFromRect $xMin [expr $vdd_y - $width / 2] $xMax [expr $vdd_y + $width / 2]]
      set vdd_name [get_voltage_domain_power [get_voltage_domain $xMin [expr $vdd_y - $width / 2] $xMax [expr $vdd_y + $width / 2]]]
      set vss_box [::odb::newSetFromRect $xMin [expr $vss_y - $width / 2] $xMax [expr $vss_y + $width / 2]]
      set vss_name [get_voltage_domain_ground [get_voltage_domain $xMin [expr $vss_y - $width / 2] $xMax [expr $vss_y + $width / 2]]]
      # generate power_rails using first domain_power
      set first_power_name [lindex $vdd_name 0]
      set switched_power_name [get_voltage_domain_switched_power [dict get $design_data core_domain]]
      # debug "[$box xMin] [expr $vdd_y - $width / 2] [$box xMax] [expr $vdd_y + $width / 2]"
      if {$switched_power_name != ""} {
        add_stripe $lay $switched_power_name $vdd_box
      } else {
        add_stripe $lay $first_power_name $vdd_box
      }
      add_stripe $lay $vss_name $vss_box
    }
  }
}

proc starts_with {lay} {
  variable grid_data
  variable stripes_start_with

  if {[dict exists $grid_data straps $lay starts_with]} {
    set starts_with [dict get $grid_data straps $lay starts_with]
  } elseif {[dict exists $grid_data starts_with]} {
    set starts_with [dict get $grid_data starts_with]
  } else {
    set starts_with $stripes_start_with
  }

  return $starts_with
}

# proc for creating pdn mesh for upper metal layers
proc generate_upper_metal_mesh_stripes {net layer layer_info area} {
# If the grid_data defines a spacing for the layer, then:
#    place the second stripe spacing + width away from the first,
# otherwise:
#    place the second stripe pitch / 2 away from the first,
#
  variable design_data
  set width [dict get $layer_info width]
  set switched_power_name [get_voltage_domain_switched_power [dict get $design_data core_domain]]
  # debug "Starts with: [starts_with $layer]"
  # debug "net: $net, power: [is_power_net $net], ground: [is_ground_net $net], layer: $layer, area: $area"

  if {[get_dir $layer] == "hor"} {
    set offset [expr [lindex $area 1] + [dict get $layer_info offset]]
    if {$net == $switched_power_name} { #VDD_SW
      set offset1 [expr {$offset + ([dict get $layer_info pitch] / 4)}]
      set offset2 [expr {$offset - ([dict get $layer_info pitch] / 4)}]
      for {set y $offset1} {$y < [expr {[lindex $area 3] - [dict get $layer_info width]}]} {set y [expr {[dict get $layer_info pitch] + $y}]} {
        set box [::odb::newSetFromRect [lindex $area 0] [expr $y - $width / 2] [lindex $area 2] [expr $y + $width / 2]]
        add_stripe $layer $net $box
      }
      for {set y $offset2} {$y < [expr {[lindex $area 3] - [dict get $layer_info width]}]} {set y [expr {[dict get $layer_info pitch] + $y}]} {
        set box [::odb::newSetFromRect [lindex $area 0] [expr $y - $width / 2] [lindex $area 2] [expr $y + $width / 2]]
        add_stripe $layer $net $box
      }
    } else {
      if {([starts_with $layer] == "POWER" && ![is_power_net $net]) || ([starts_with $layer] == "GROUND" && ![is_ground_net $net])} {
        if {[dict exists $layer_info spacing]} {
          set offset [expr {$offset + [dict get $layer_info spacing] + [dict get $layer_info width]}]
        } else {
          set offset [expr {$offset + ([dict get $layer_info pitch] / 2)}]
        }
      }

      for {set y $offset} {$y < [expr {[lindex $area 3] - [dict get $layer_info width]}]} {set y [expr {[dict get $layer_info pitch] + $y}]} {
        set box [::odb::newSetFromRect [lindex $area 0] [expr $y - $width / 2] [lindex $area 2] [expr $y + $width / 2]]
        # debug "  box: [lindex $area 0] [expr $y - $width / 2] [lindex $area 2] [expr $y + $width / 2]"
        add_stripe $layer $net $box
      }
    }
  } elseif {[get_dir $layer] == "ver"} {
    set offset [expr [lindex $area 0] + [dict get $layer_info offset]]
    # debug "layer_offset: [dict get $layer_info offset]"
    # debug "base_offset: $offset"
    if {$net == $switched_power_name} { #VDD_SW
      set offset1 [expr {$offset + ([dict get $layer_info pitch] / 4)}]
      set offset2 [expr {$offset - ([dict get $layer_info pitch] / 4)}]
      for {set x $offset1} {$x < [expr {[lindex $area 2] - [dict get $layer_info width]}]} {set x [expr {[dict get $layer_info pitch] + $x}]} {
        set box [::odb::newSetFromRect [expr $x - $width / 2] [lindex $area 1] [expr $x + $width / 2] [lindex $area 3]]
        add_stripe $layer $net $box
      }
      for {set x $offset2} {$x < [expr {[lindex $area 2] - [dict get $layer_info width]}]} {set x [expr {[dict get $layer_info pitch] + $x}]} {
        set box [::odb::newSetFromRect [expr $x - $width / 2] [lindex $area 1] [expr $x + $width / 2] [lindex $area 3]]
        add_stripe $layer $net $box
      }
    } else {
      if {([starts_with $layer] == "POWER" && ![is_power_net $net]) || ([starts_with $layer] == "GROUND" && ![is_ground_net $net])} {
        set offset [expr [lindex $area 0] + [dict get $layer_info offset]]
        if {[dict exists $layer_info spacing]} {
          set offset [expr {$offset + [dict get $layer_info spacing] + [dict get $layer_info width]}]
          # debug "spacing_based_offset: $offset"
        } else {
          set offset [expr {$offset + ([dict get $layer_info pitch] / 2)}]
          # debug "pitch_based_offset: $offset"
        }
      }
      for {set x $offset} {$x < [expr {[lindex $area 2] - [dict get $layer_info width]}]} {set x [expr {[dict get $layer_info pitch] + $x}]} {
        set box [::odb::newSetFromRect [expr $x - $width / 2] [lindex $area 1] [expr $x + $width / 2] [lindex $area 3]]
        # debug "  box: [expr $x - $width / 2] [lindex $area 1] [expr $x + $width / 2] [lindex $area 3]"
        add_stripe $layer $net $box
      }
    }
  } else {
    utl::error PDN 9026 "Invalid direction \"[get_dir $layer]\" for metal layer ${layer}. Should be either \"hor\" or \"ver\"."
  }
}

proc adjust_area_for_core_rings {layer area number} {
  variable grid_data

  # When core_rings overlap with the stdcell area, we need to block out the area
  # where the core rings have been placed.
  if {[dict exists $grid_data core_ring_area [get_core_domain_name] $layer]} {
    set core_ring_area [dict get $grid_data core_ring_area [get_core_domain_name] $layer]
    set grid_area [odb::newSetFromRect {*}$area]
    set grid_area [odb::subtractSet $grid_area $core_ring_area]
    set init_area $area
    set area [get_extent $grid_area]
    # debug "init_area: $init_area, final_area: $area"
    # debug "core_ring_area:"
    foreach rect [odb::getRectangles $core_ring_area] {
      # debug "  rect: [$rect xMin] [$rect yMin] [$rect xMax] [$rect yMax]"
    }
  }

  # Calculate how far to extend the grid to meet with the core rings
  if {[dict exists $grid_data core_ring $layer pad_offset]} {
    set pad_area [find_pad_offset_area]
    set width [dict get $grid_data core_ring $layer width]
    set offset [expr [dict get $grid_data core_ring $layer pad_offset]]
    set spacing [dict get $grid_data core_ring $layer spacing]
    set xMin [expr [lindex $pad_area 0] + $offset]
    set yMin [expr [lindex $pad_area 1] + $offset]
    set xMax [expr [lindex $pad_area 2] - $offset]
    set yMax [expr [lindex $pad_area 3] - $offset]
  } elseif {[dict exists $grid_data core_ring $layer core_offset]} {
    set offset [dict get $grid_data core_ring $layer core_offset]
    set width [dict get $grid_data core_ring $layer width]
    set spacing [dict get $grid_data core_ring $layer spacing]
    # debug "Area: $area"
    # debug "Offset: $offset, Width $width, Spacing $spacing, #rings: $number"

    # The area figure includes a y offset for the width of the stdcell rail - so need to subtract it here
    set rail_width [get_rails_max_width]

    # set extension area according to the number of power rings of the voltage domain, the default number is 2
    set xMin [expr [lindex $area 0] - $offset - $width / 2 - ($number - 1) * ($width + $spacing)]
    set yMin [expr [lindex $area 1] - $offset - $width / 2 + $rail_width / 2 - ($number - 1) * ($width + $spacing)]
    set xMax [expr [lindex $area 2] + $offset + $width / 2 + ($number - 1) * ($width + $spacing)]
    set yMax [expr [lindex $area 3] + $offset + $width / 2 - $rail_width / 2 + ($number - 1) * ($width + $spacing)]
  }
  if {[get_dir $layer] == "hor"} {
    set extended_area [list $xMin [lindex $area 1] $xMax [lindex $area 3]]
  } else {
    set extended_area [list [lindex $area 0] $yMin [lindex $area 2] $yMax]
  }
  return $extended_area
}

proc is_lowest_strap {lay} {
  variable grid_data

  foreach other_lay [dict keys [dict get $grid_data straps]] {
    if {[get_layer_number $other_lay] < [get_layer_number $lay]} {
      return 0
    }
  }
  return 1 
}

## this is a top-level proc to generate PDN stripes and insert vias between these stripes
proc generate_stripes {net_name} {
  variable plan_template
  variable template
  variable grid_data
  variable block
  variable design_data
  variable voltage_domains

  # debug "start: net: $net_name, grid_name: [dict get $grid_data name]"
  if {![dict exists $grid_data straps]} {return}
  foreach lay [dict keys [dict get $grid_data straps]] {
    # debug "    Layer $lay ..."
    #Upper layer stripes
    if {[dict exists $grid_data straps $lay width]} {
      set area [dict get $grid_data area]
      # debug "Area $area"
      # Calculate the number of rings of core_domain
      set ring_number [llength [get_pg_nets]]
      if {[dict exists $grid_data core_ring] && [dict exists $grid_data core_ring $lay]} {
        set area [adjust_area_for_core_rings $lay $area $ring_number]
      } else {
        # debug "no adjustment"
        # debug $grid_data
      }
      # debug "area: $area (spec area: [dict get $grid_data area])"
      # Create stripes for core domain's pwr/gnd nets

      if {$net_name == [get_voltage_domain_power [dict get $design_data core_domain]] ||
          $net_name == [get_voltage_domain_ground [dict get $design_data core_domain]]} {
        generate_upper_metal_mesh_stripes $net_name $lay [dict get $grid_data straps $lay] $area
        # Split core domains pwr/gnd nets when they cross other voltage domains that have different pwr/gnd nets
        update_mesh_stripes_with_voltage_domains $net_name $lay
      }

      if {$net_name == [get_voltage_domain_switched_power [dict get $design_data core_domain]]} {
        if {[is_lowest_strap $lay]} {
          # debug "Generate $net_name on layer $lay"
          generate_upper_metal_mesh_stripes $net_name $lay [dict get $grid_data straps $lay] $area
	}
      }

      # Create stripes for each voltage domains
      foreach domain_name [dict keys $voltage_domains] {
        if {$domain_name == [dict get $design_data core_domain]} {continue}
        set domain [$block findRegion $domain_name]
        set rect [lindex [$domain getBoundaries] 0]
        set domain_name [$domain getName]
        set domain_xMin [$rect xMin]
        set domain_yMin [$rect yMin]
        set domain_xMax [$rect xMax]
        set domain_yMax [$rect yMax]
        set width [dict get $grid_data core_ring $lay width]
        set spacing [dict get $grid_data core_ring $lay spacing]
        set rail_width [get_rails_max_width]
        # debug "domain: $domain_name, domain_area: [$rect xMin] [$rect yMin] [$rect xMax] [$rect yMax], width: $width, spacing: $spacing"
        # Do not create duplicate stripes if the voltage domain has the same pwr/gnd nets as the core domain
        if {($net_name == [get_voltage_domain_power $domain_name] && $net_name != [get_voltage_domain_power [dict get $design_data core_domain]]) ||
             ($net_name == [get_voltage_domain_ground $domain_name] && $net_name != [get_voltage_domain_ground [dict get $design_data core_domain]])} {
          set area [list $domain_xMin [expr $domain_yMin - $rail_width / 2] $domain_xMax [expr $domain_yMax + $rail_width / 2]]
          set area [adjust_area_for_core_rings $lay $area 2]
          generate_upper_metal_mesh_stripes $net_name $lay [dict get $grid_data straps $lay] $area
        }
        if {[lsearch -exact [get_voltage_domain_secondary_power $domain_name] $net_name] > -1} {
          #Calculate the ring number of power_domain
          set ring_number [lsearch -exact [get_voltage_domain_secondary_power $domain_name] $net_name]
          set area [list [expr $domain_xMin + $ring_number * ($width + $spacing)] [expr $domain_yMin - $rail_width / 2] [expr $domain_xMax + $ring_number * ($width + $spacing)] [expr $domain_yMax + $rail_width / 2]]
          set area [adjust_area_for_core_rings $lay $area [expr 3 + $ring_number]]
          generate_upper_metal_mesh_stripes $net_name $lay [dict get $grid_data straps $lay] $area
        }
      }
    } else {
      foreach x [lsort -integer [dict keys $plan_template]] {
        foreach y [lsort -integer [dict keys [dict get $plan_template $x]]] {
          set template_name [dict get $plan_template $x $y]
          set layer_info [dict get $grid_data straps $lay $template_name]
          set area [list $x $y [expr $x + [dict get $template width]] [expr $y + [dict get $template height]]]
          generate_upper_metal_mesh_stripes $net_name $lay $layer_info $area
        }
      }
    }
  }
}

proc insert_power_switches {} {
  variable grid_data
  variable power_switch_cells
  variable psw_instance

  set db [ord::get_db]
  set block [ord::get_db_block]

  set name [dict get $grid_data switch_cell]
  set psw [$db findMaster $name]
  # The selected power switch is double height, and has a central VSS pin, so align with VSS rails
  set power [dict get $power_switch_cells $name power]
  set ground [dict get $power_switch_cells $name ground]
  set vgnd [[$psw findMTerm $ground] getMPins]
  set vddg [lindex [[$psw findMTerm $power] getMPins] 0]
  set vddg_lay [[[$vddg getGeometry] getTechLayer] getName]

  set rail_lay [lindex [get_rails_layers] 0]
  set rail_width [dict get $grid_data rails $rail_lay width]

  set strap_lay [lindex [dict get $grid_data straps] 0]
  set strap_lay_info [dict get $grid_data straps $strap_lay]
  set strap_width [dict get $strap_lay_info width]

  set area [dict get $grid_data area]
  set offset [expr [lindex $area 0] + [dict get $strap_lay_info offset]]

  set row_idx 0
  set psw_instance {}
  set num_crossing [expr ([llength [$block getRows]] - 1) / 2]

  foreach row [$block getRows] {
    set orient [$row getOrient]
    set box [$row getBBox]
    if {$orient == "R0"} {
      if { [expr $row_idx % 2] == 1 || $row_idx == $num_crossing} {
        if { [expr $num_crossing % 2] == 1 && $row_idx == $num_crossing} {continue}
        set vss_y [$box yMin]
        set overlap_y [expr $vss_y - $rail_width / 2]
        
        set col_idx 0

        for {set x $offset} {$x < [expr {[lindex $area 2] - [dict get $strap_lay_info width]}]} {set x [expr {[dict get $strap_lay_info pitch] + $x}]} {
          set overlap_x [expr $x - $strap_width / 2]

          set location [list \
            [expr $overlap_x - [[$vddg getBBox] xMin]] \
            [expr $overlap_y - [[$vgnd getBBox] yMin]] \
          ]

          set inst_name "PSW_${row_idx}_${col_idx}"
          if {[set inst [$block findInst $inst_name]] == "NULL"} {
            set inst [odb::dbInst_create $block $psw $inst_name]
          }

          $inst setOrigin {*}$location
          $inst setPlacementStatus "FIRM"

          lappend psw_instance $inst
          incr col_idx
        }
      }

      incr row_idx
    }
  }

  # automatically add design connection rule for vddg pin layer
  set connections {}
  foreach connection [dict get $grid_data connect] {
    set l1 [lindex $connection 0]
    set l2 [lindex $connection 1]
    lappend connections $connection
    if {$l1 == $vddg_lay} {
      lappend connections "${l1}_PIN_hor ${l2}"
    } elseif {$l2 == $vddg_lay} {
      lappend connections "${l1} ${l2}_PIN_hor"
    }
  }
  dict set grid_data connect $connections

  foreach inst $psw_instance {
    set orient [$inst getOrient]
    set origin [$inst getOrigin]
    foreach vddg [$inst findITerm $power] {
      set mTerm [$vddg getMTerm]
      foreach mPin [$mTerm getMPins] {
        set shape [$mPin getGeometry]
        set layer [[$shape getTechLayer] getName]
        set rect [transform_box [$shape xMin] [$shape yMin] [$shape xMax] [$shape yMax] $origin $orient]
        # debug "layer: $layer, rect: $rect"
        add_stripe "${layer}_PIN_hor" "VDD" [odb::newSetFromRect {*}$rect]
      }
      # set vddg_layer [[[$vddg getGeometry] getTechLayer] getName]
      # add_stripe $vddg_layer "POWER" [odb::newSetFromRect [[$vddg getBBox] xMin] [[$vddg getBBox] yMin] [[$vddg getBBox] xMax] [[$vddg getBBox] yMax]]
    }
  }
  merge_stripes
}

proc get_power_switch_shape {} {
  variable psw_instance
  set psw {}

  foreach psw_inst $psw_instance {
    set bbox [$psw_inst getBBox]
    set box [odb::newSetFromRect [$bbox xMin] [$bbox yMin] [$bbox xMax] [$bbox yMax]]
    lappend psw $box
  }
  return [odb::orSets $psw]
}
 
proc move_tapcell {tapcell_inst switch_inst print_num} {

  set tapcell_bbox [$tapcell_inst getBBox]
  set switch_bbox [odb::getRectangles $switch_inst]

  set avg_tapcell [expr ([$tapcell_bbox xMin] + [$tapcell_bbox xMax]) / 2]
  set avg_switch [expr ([$switch_bbox xMin] + [$switch_bbox xMax]) / 2]

  if {$avg_tapcell > $avg_switch} { 
    # move to rhs
    set location [list \
            [expr [lindex [$tapcell_inst getOrigin] 0] + [$switch_bbox xMax] - [$tapcell_bbox xMin]] \
            [lindex [$tapcell_inst getOrigin] 1] \
          ]
  } else {
    # move to lhs
    set location [list \
            [expr [lindex [$tapcell_inst getOrigin] 0] - [$tapcell_bbox xMax] + [$switch_bbox xMin]] \
            [lindex [$tapcell_inst getOrigin] 1] \
          ]
  }
  $tapcell_inst setOrigin {*}$location
}

proc detect_and_fix_tapcell_overlaps {} {
  variable grid_data

  set db [ord::get_db]
  set block [ord::get_db_block]

  set name [dict get $grid_data switch_cell]
  set psw_width [[$db findMaster $name] getWidth]

  set power_switches [get_power_switch_shape]
  set tapcell_idx 0

  foreach Inst [$block getInsts] {
    set tapcell_inst [$block findInst "TAP_${tapcell_idx}"]
    if {$tapcell_inst != "NULL"} {
      set bbox [$tapcell_inst getBBox]
      set box [odb::newSetFromRect [$bbox xMin] [$bbox yMin] [$bbox xMax] [$bbox yMax]]
      set overlap [odb::andSet $power_switches $box]

      if {[llength [odb::getRectangles $overlap]] > 0} {
        set switch_overlap [odb::bloatSet $overlap $psw_width]
        set switch_inst  [odb::andSet $power_switches $switch_overlap]
        move_tapcell $tapcell_inst $switch_inst $tapcell_idx
      } 
      incr tapcell_idx
    } else {
      break
    }
  }
}

proc is_power_control_star {} {
  variable grid_data
  if {![dict exists $grid_data power_control_network]} {
    dict set grid_data power_control_network "STAR"
  }
  if {[dict get $grid_data power_control_network] == "STAR"} {
    return 1
  }
  return 0
}

proc is_power_control_daisy_chain {} {
  variable grid_data
  if {![dict exists $grid_data power_control_network]} {
    dict set grid_data power_control_network "STAR"
  }
  if {[dict get $grid_data power_control_network] == "DAISY"} {
    return 1
  }
  return 0
}

proc connect_power_switches {} {
  variable power_switch_cells
  variable grid_data

  set block [ord::get_db_block]
  if {![dict exists $grid_data power_control]} {
    utl::error PDN 9191 "No power control signal is defined for a grid that includes power switches"
  }
  set power_control_signal [dict get $grid_data power_control]
  set power_control_net [$block findNet $power_control_signal]
  if {$power_control_net == "NULL"} {
    utl::error PDN 9192 "Cannot find power control signal [dict get $power_switch control_signal]"
  }

  if {[is_power_control_star]} {
    # Connect all the power control signal inputs to the power control signal in a high fanout net configuration
    foreach inst [$block getInsts] {
      set cell_name [[$inst getMaster] getName]
      if {![dict exists $power_switch_cells $cell_name]} {continue}

      set control_pin [dict get $power_switch_cells $cell_name control]
      if {[set iterm [$inst findITerm $control_pin]] != "NULL"} {
        $iterm connect $power_control_net
      } else {
        utl::error PDN 9193 "Cannot find instance term $control_pin for [$inst getName] of cell [[$inst getMaster] getName]"
      }
    }
  } elseif {[is_power_control_daisy_chain]} {
    foreach inst [$block getInsts] {
      set cell_name [[$inst getMaster] getName]
      if {[dict exists $power_switch_cells $cell_name]} {
	lappend psw_instances $inst
      }
    }
    # sort instances by x, then y
    foreach inst $psw_instances {
      lassign [$inst getOrigin] x y
      dict lappend columns $x $inst
    }

    set control_net $power_control_net
    foreach x [lsort -integer [dict keys $columns]] {
      set col [dict get $columns $x]
      set sorted_col [lreverse [lsort -command sort_insts_by_y $col]]
      foreach inst $sorted_col {
	# debug "inst x=[lindex [$inst getOrigin] 0], y=[lindex [$inst getOrigin] 1]"
        set control_pin [$inst findITerm [dict get $power_switch_cells $cell_name control]]
        $control_pin connect $control_net
        set control_net [odb::dbNet_create $block "[$inst getName]_out"]
	set ack_pin_name [dict get $power_switch_cells $cell_name acknowledge]
        set ack_pin [$inst findITerm $ack_pin_name]
	if {$ack_pin == "NULL"} {
          utl::error PDN 9197 "Cannot find pin $ack_pin_name on power switch [$inst getName] ($cell_name)"
	}
        $ack_pin connect $control_net
      }
      set first [lindex $sorted_col 0]
      set control_net [$block findNet "[$first getName]_out"]
      # debug "control_net=[$control_net getName]"
    }
  } else {
    utl::error PDN 9196 "Invalid value specified for power control network type"
  }
}



proc cut_blocked_areas {net} {
  variable stripe_locs
  variable grid_data

  if {![dict exists  $grid_data straps]} {return}

  foreach layer_name [dict keys [dict get $grid_data straps]] {
    set width [get_grid_wire_width $layer_name]

    set blockages [get_blockages]
    if {[dict exists $blockages $layer_name]} {
      if {[array names stripe_locs "$layer_name,$net"] != ""} {
        set stripe_locs($layer_name,$net) [::odb::subtractSet $stripe_locs($layer_name,$net) [dict get $blockages $layer_name]]

        # Trim any shapes that are less than the width of the wire
        set size_by [expr $width / 2 - 1]
        set trimmed_set [::odb::shrinkSet $stripe_locs($layer_name,$net) $size_by]
        set stripe_locs($layer_name,$net) [::odb::bloatSet $trimmed_set $size_by]
      }
    }
  }
}

proc generate_grid_vias {net_name} {
  variable vias
  variable grid_data
  variable design_data

  #Via stacks
  # debug "grid_data $grid_data"
  if {[dict exists $grid_data connect]} {
    # debug "Adding vias for $net_name ([llength [dict get $grid_data connect]] connections)..."
    foreach connection [dict get $grid_data connect] {
        set l1 [lindex $connection 0]
        set l2 [lindex $connection 1]
        # debug "    $l1 to $l2"
        set connections [generate_via_stacks $l1 $l2 $net_name $connection]
        lappend vias [list net_name $net_name connections $connections]
    }
  }
  # debug "End"
}

proc get_core_ring_centre {type side layer_info} {
  variable grid_data

  set spacing [dict get $layer_info spacing]
  set width [dict get $layer_info width]

  if {[dict exists $layer_info pad_offset]} {
    set area [find_pad_offset_area]
    lassign $area xMin yMin xMax yMax
    set offset [expr [dict get $layer_info pad_offset] + $width / 2]
    # debug "area        $area"
    # debug "pad_offset  $offset"
    # debug "spacing     $spacing"
    # debug "width       $width"
    switch $type {
      "GROUND" {
        switch $side {
          "t" {return [expr $yMax - $offset]}
          "b" {return [expr $yMin + $offset]}
          "l" {return [expr $xMin + $offset]}
          "r" {return [expr $xMax - $offset]}
        }
      }
      "POWER" {
        switch $side {
          "t" {return [expr $yMax - $offset - $spacing - $width]}
          "b" {return [expr $yMin + $offset + $spacing + $width]}
          "l" {return [expr $xMin + $offset + $spacing + $width]}
          "r" {return [expr $xMax - $offset - $spacing - $width]}
        }
      }
    }
  } elseif {[dict exists $layer_info core_offset]} {
    set area [find_core_area]
    set xMin [lindex $area 0]
    set yMin [lindex $area 1]
    set xMax [lindex $area 2]
    set yMax [lindex $area 3]

    set offset [dict get $layer_info core_offset]
    # debug "area        $area"
    # debug "core_offset $offset"
    # debug "spacing     $spacing"
    # debug "width       $width"
    switch $type {
      "POWER" {
        switch $side {
          "t" {return [expr $yMax + $offset]}
          "b" {return [expr $yMin - $offset]}
          "l" {return [expr $xMin - $offset]}
          "r" {return [expr $xMax + $offset]}
        }
      }
      "GROUND" {
        switch $side {
          "t" {return [expr $yMax + $offset + $spacing + $width]}
          "b" {return [expr $yMin - $offset - $spacing - $width]}
          "l" {return [expr $xMin - $offset - $spacing - $width]}
          "r" {return [expr $xMax + $offset + $spacing + $width]}
        }
      }
    }
  }
}

proc find_pad_offset_area {} {
  variable block
  variable grid_data
  variable design_data

  if {!([dict exists $grid_data pwr_pads] && [dict exists $grid_data gnd_pads])} {
    utl::error PDN 9048 "Need to define pwr_pads and gnd_pads in config file to use pad_offset option."
  }

  if {![dict exists $design_data config pad_offset_area]} {
    set pad_names {}
    dict for {pin_name pads} [dict get $grid_data pwr_pads] {
      set pad_names [concat $pad_names $pads]
    }
    dict for {pin_name pads} [dict get $grid_data gnd_pads] {
      set pad_names [concat $pad_names $pads]
    }
    set pad_names [lsort -unique $pad_names]
    set die_area [dict get $design_data config die_area]
    set xMin [lindex $die_area 0]
    set yMin [lindex $die_area 1]
    set xMax [lindex $die_area 2]
    set yMax [lindex $die_area 3]

    # debug "pad_names: $pad_names"
    set found_b 0
    set found_r 0
    set found_t 0
    set found_l 0
    foreach inst [$block getInsts] {
      if {[lsearch $pad_names [[$inst getMaster] getName]] > -1} {
        # debug "inst_master: [[$inst getMaster] getName]"
        set quadrant [get_design_quadrant {*}[$inst getOrigin]]
        switch $quadrant {
          "b" {
            # debug "inst: [$inst getName], side: $quadrant, yMax: [ord::dbu_to_microns [[$inst getBBox] yMax]]"
            set found_b 1
            if {$yMin < [set y [[$inst getBBox] yMax]]} {
              set yMin $y
            }
          }
          "r" {
            # debug "inst: [$inst getName], side: $quadrant, xMin: [ord::dbu_to_microns [[$inst getBBox] xMin]]"
            set found_r 1
            if {$xMax > [set x [[$inst getBBox] xMin]]} {
              set xMax $x
            }
          }
          "t" {
            # debug "inst: [$inst getName], side: $quadrant, yMin: [ord::dbu_to_microns [[$inst getBBox] yMin]]"
            set found_t 1
            if {$yMax > [set y [[$inst getBBox] yMin]]} {
              set yMax $y
            }
          }
          "l" {
            # debug "inst: [$inst getName], side: $quadrant, xMax: [ord::dbu_to_microns [[$inst getBBox] xMax]]"
            set found_l 1
            if {$xMin < [set x [[$inst getBBox] xMax]]} {
              set xMin $x
            }
          }
        }
      }
    }
    if {$found_b == 0} {
      utl::warn PDN 9064 "No power/ground pads found on bottom edge."
    }
    if {$found_r == 0} {
      utl::warn PDN 9065 "No power/ground pads found on right edge."
    }
    if {$found_t == 0} {
      utl::warn PDN 9066 "No power/ground pads found on top edge."
    }
    if {$found_l == 0} {
      utl::warn PDN 9067 "No power/ground pads found on left edge."
    }
    if {$found_b == 0 || $found_r == 0 || $found_t == 0 || $found_l == 0} {
      utl::error PDN 9068 "Cannot place core rings without pwr/gnd pads on each side."
    }
    # debug "pad_area: ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin]) ([ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax])"
    dict set design_data config pad_offset_area [list $xMin $yMin $xMax $yMax]
  }

  return [dict get $design_data config pad_offset_area]
}

proc add_core_rings {net layer width lx ly ux uy} {
  set core_rings {}
  if {[get_dir $layer] == "hor"} {
    # debug              "layer: $layer, net: $net, [expr $lx - $width / 2] [expr $ly - $width / 2] [expr $ux + $width / 2] [expr $ly + $width / 2]"
    # debug              "layer: $layer, net: $net, [expr $lx - $width / 2] [expr $uy - $width / 2] [expr $ux + $width / 2] [expr $uy + $width / 2]"
    set lower [odb::newSetFromRect [expr $lx - $width / 2] [expr $ly - $width / 2] [expr $ux + $width / 2] [expr $ly + $width / 2]]
    set upper [odb::newSetFromRect [expr $lx - $width / 2] [expr $uy - $width / 2] [expr $ux + $width / 2] [expr $uy + $width / 2]]
  } else {
    # debug              "layer: $layer, net: $net, [expr $lx - $width / 2] [expr $ly - $width / 2] [expr $lx + $width / 2] [expr $uy + $width / 2]"
    # debug              "layer: $layer, net: $net, [expr $ux - $width / 2] [expr $ly - $width / 2] [expr $ux + $width / 2] [expr $uy + $width / 2]"
    set lower [odb::newSetFromRect [expr $lx - $width / 2] [expr $ly - $width / 2] [expr $lx + $width / 2] [expr $uy + $width / 2]]
    set upper [odb::newSetFromRect [expr $ux - $width / 2] [expr $ly - $width / 2] [expr $ux + $width / 2] [expr $uy + $width / 2]]
  }
  add_stripe $layer $net $lower
  add_stripe $layer $net $upper
  lappend core_rings $lower
  lappend core_rings $upper

  return [odb::orSets $core_rings]
}

proc use_pad_offset {core_ring_data} {
  dict for {layer layer_info} $core_ring_data {
    if {[dict exists $layer_info pad_offset]} {
      return 1
    }
  }
  return 0
}

proc generate_core_rings {core_ring_data} {
  variable design_data
  variable block
  variable grid_data
  variable voltage_domains

  foreach domain_name [dict keys $voltage_domains] {
    set ring_areas {}
    set pg_nets [get_pg_nets $domain_name]
    # debug "domain: $domain_name, pg_nets: $pg_nets"

    if {$domain_name == [dict get $design_data core_domain]} {
      if {[use_pad_offset $core_ring_data]} {
        set area [find_pad_offset_area]
        set pg_nets [lreverse $pg_nets]
      } else {
        set area [list {*}[[ord::get_db_core] ll] {*}[[ord::get_db_core] ur]]
      }
    } else {
      set domain [$block findRegion $domain_name]
      set rect [lindex [$domain getBoundaries] 0]
      set area [list [$rect xMin] [$rect yMin] [$rect xMax] [$rect yMax]]
    }

    dict for {layer layer_info} $core_ring_data {
      set spacing [dict get $layer_info spacing]
      set width [dict get $layer_info width]

      if {[use_pad_offset $core_ring_data]} {
        # debug "pad_offset"
        set offset [expr [dict get $layer_info pad_offset]]

        set lx [expr [lindex $area 0] + $offset + $width / 2]
        set ly [expr [lindex $area 1] + $offset + $width / 2]
        set ux [expr [lindex $area 2] - $offset - $width / 2]
        set uy [expr [lindex $area 3] - $offset - $width / 2]
      } else {
        # debug "core_offset"
        set offset [expr [dict get $layer_info core_offset]]

        set lx [expr [lindex $area 0] - $offset]
        set ly [expr [lindex $area 1] - $offset]
        set ux [expr [lindex $area 2] + $offset]
        set uy [expr [lindex $area 3] + $offset]
      }

      set init_lx $lx
      set init_ly $ly
      set init_ux $ux
      set init_uy $uy
      if {[use_pad_offset $core_ring_data]} {
        set final_lx [expr $lx + ([llength $pg_nets] - 1) * ($width + $spacing)]
        set final_ly [expr $ly + ([llength $pg_nets] - 1) * ($width + $spacing)]
        set final_ux [expr $ux - ([llength $pg_nets] - 1) * ($width + $spacing)]
        set final_uy [expr $uy - ([llength $pg_nets] - 1) * ($width + $spacing)]
      } else {
        set final_lx [expr $lx - ([llength $pg_nets] - 1) * ($width + $spacing)]
        set final_ly [expr $ly - ([llength $pg_nets] - 1) * ($width + $spacing)]
        set final_ux [expr $ux + ([llength $pg_nets] - 1) * ($width + $spacing)]
        set final_uy [expr $uy + ([llength $pg_nets] - 1) * ($width + $spacing)]
      }

      foreach net $pg_nets {
        add_core_rings $net $layer $width $lx $ly $ux $uy
        # debug "ring - net: $net, layer: $layer, width: $width, area: $lx $ly $ux $uy"

        if {[use_pad_offset $core_ring_data]} {
          set lx [expr $lx + $spacing + $width]
          set ly [expr $ly + $spacing + $width]
          set ux [expr $ux - $spacing - $width]
          set uy [expr $uy - $spacing - $width]
        } else {
          set lx [expr $lx - $spacing - $width]
          set ly [expr $ly - $spacing - $width]
          set ux [expr $ux + $spacing + $width]
          set uy [expr $uy + $spacing + $width]
        }
      }

      if {[get_dir $layer] == "hor"} {
        set lower [odb::newSetFromRect [expr min($init_lx,$final_lx) - $width / 2] [expr min($init_ly,$final_ly) - $width / 2] [expr max($init_ux,$final_ux) + $width / 2] [expr max($init_ly,$final_ly) + $width / 2]]
        set upper [odb::newSetFromRect [expr min($init_lx,$final_lx) - $width / 2] [expr min($init_uy,$final_uy) - $width / 2] [expr max($init_ux,$final_ux) + $width / 2] [expr max($init_uy,$final_uy) + $width / 2]]
      } else {
        set lower [odb::newSetFromRect [expr min($init_lx,$final_lx) - $width / 2] [expr min($init_ly,$final_ly) - $width / 2] [expr max($init_lx,$final_lx) + $width / 2] [expr max($init_uy,$final_uy) + $width / 2]]
        set upper [odb::newSetFromRect [expr min($init_ux,$final_ux) - $width / 2] [expr min($init_ly,$final_ly) - $width / 2] [expr max($init_ux,$final_ux) + $width / 2] [expr max($init_uy,$final_uy) + $width / 2]]
      }
      set core_ring_area [odb::bloatSet [odb::orSets [list $upper $lower]] $spacing]
      # set core_ring_area [odb::bloatSet $core_rings [expr $width + $spacing]]
      # debug "domain_name: $domain_name, layer: $layer, area: $area, offset: $offset, pg_nets: $pg_nets, spacing: $spacing, width: $width"
      # debug "core_ring_area:"
      foreach rect [odb::getRectangles $core_ring_area] {
        # debug "  rect [$rect xMin] [$rect yMin] [$rect xMax] [$rect yMax]"
      }
      lappend ring_areas $core_ring_area
      dict set grid_data core_ring_area $domain_name $layer $core_ring_area

    }
    dict set grid_data core_ring_area $domain_name combined [odb::orSets $ring_areas]
  }
}

proc report_rect {rect} {
  return "[$rect xMin] [$rect yMin] [$rect xMax] [$rect yMax]"
}

proc report_shapes {net layer} {
  variable stripe_locs
  merge_stripes

  set report ""
  if {[array names stripe_locs "$layer,$net"] != ""} {
    foreach sh $stripe_locs($layer,$net) {
      set report "net: $net, layer $layer"
      foreach rect [odb::getRectangles $sh] {
        set report "$report\n  [report_rect $rect]"
      }
    }
  }
  return $report
}

proc get_macro_boundaries {} {
  variable instances

  set boundaries {}
  oreach instance [dict keys $instances] {
    lappend boundaries [dict get $instances $instance macro_boundary]
  }

  return $boundaries
}

proc get_stdcell_specification {} {
  variable design_data

  if {[dict exists $design_data grid stdcell]} {
    set grid_name [lindex [dict keys [dict get $design_data grid stdcell]] 0]
    return [dict get $design_data grid stdcell $grid_name]
  } else {
    if {![dict exists $design_data grid stdcell]} {
      utl::error PDN 9017 "No stdcell grid specification found - no rails can be inserted."
    }
  }

  return {}
}

proc get_rail_width {} {
  variable default_grid_data

  set max_width 0
  foreach layer [get_rails_layers] {
    set max_width [expr max($max_width,[get_grid_wire_width $layer])]
  }
  if {![dict exists $default_grid_data units]} {
    set max_width [ord::microns_to_dbu $max_width]
  }
  return $max_width
}


proc get_macro_blocks {} {
  variable macros

  if {[llength $macros] > 0} {return $macros}

  # debug "start"
  foreach lib [[ord::get_db] getLibs] {
    foreach cell [$lib getMasters] {
      if {![$cell isBlock] && ![$cell isPad]} {continue}
      set macro_name [$cell getName]
      dict set macros $macro_name width  [$cell getWidth]
      dict set macros $macro_name height [$cell getHeight]

      set blockage_layers {}
      foreach obs [$cell getObstructions] {
        set layer_name [[$obs getTechLayer] getName]
        dict set blockage_layers $layer_name 1
      }
      dict set macros $macro_name blockage_layers [dict keys $blockage_layers]

      set pin_layers {}
      set power_pins {}
      set ground_pins {}
      set first_shape 1

      foreach term [$cell getMTerms] {
        set sig_type [$term getSigType]
        if {$sig_type == "POWER"} {
          lappend power_pins [$term getName]
        } elseif {$sig_type == "GROUND"} {
          lappend ground_pins [$term getName]
        } else {
          continue
        }

        foreach pin [$term getMPins] {
          foreach shape [$pin getGeometry] {
            lappend pin_layers [[$shape getTechLayer] getName]
            if {$first_shape == 1} {
              set xMin [$shape xMin]
              set xMax [$shape xMax]
              set yMin [$shape yMin]
              set yMax [$shape yMax]
              set first_shape 0
            } else {
              set xMin [expr min($xMin,[$shape xMin])]
              set xMax [expr max($xMax,[$shape xMax])]
              set yMin [expr min($yMin,[$shape yMin])]
              set yMax [expr max($yMax,[$shape yMax])]
            }
          }
        }
      }

      dict set macros $macro_name pin_layers [lsort -unique $pin_layers]
      dict set macros $macro_name power_pins [lsort -unique $power_pins]
      dict set macros $macro_name ground_pins [lsort -unique $ground_pins]
      if {$first_shape == 0}  {
        dict set macros $macro_name pins_area [list $xMin $yMin $xMax $yMax]
      } else {
        dict set macros $macro_name pins_area [list 0 0 0 0]
      }
    }
  }

  return $macros
}

proc filtered_insts_within {instances boundary} {
  set filtered_instances {}
  dict for {instance_name instance} $instances {
  # If there are no shapes left after 'and'ing the boundard with the cell, then
  # the cell lies outside the area where we are adding a power grid.
    set llx [dict get $instance xmin]
    set lly [dict get $instance ymin]
    set urx [dict get $instance xmax]
    set ury [dict get $instance ymax]

    set box [odb::newSetFromRect $llx $lly $urx $ury]
    if {[llength [odb::getPolygons [odb::andSet $boundary $box]]] != 0} {
      dict set filtered_instances $instance_name $instance
    }
  }
  return $filtered_instances
}

proc import_macro_boundaries {} {
  variable libs
  variable instances

  set macros [get_macro_blocks]
  set instances [find_instances_of [dict keys $macros]]

  # debug "end"
}

proc get_instances {} {
  variable instances

  if {[llength $instances] > 0} {return $instances}

  set block [ord::get_db_block]
  foreach inst [$block getInsts] {
    if {![[$inst getMaster] isBlock] && ![[$inst getMaster] isPad]} {continue}
    set instance {}
    dict set instance name [$inst getName]
    dict set instance inst $inst
    dict set instance macro [[$inst getMaster] getName]
    dict set instance x [lindex [$inst getOrigin] 0]
    dict set instance y [lindex [$inst getOrigin] 1]
    dict set instance xmin [[$inst getBBox] xMin]
    dict set instance ymin [[$inst getBBox] yMin]
    dict set instance xmax [[$inst getBBox] xMax]
    dict set instance ymax [[$inst getBBox] yMax]
    dict set instance orient [$inst getOrient]


    set llx [dict get $instance xmin]
    set lly [dict get $instance ymin]
    set urx [dict get $instance xmax]
    set ury [dict get $instance ymax]
    dict set instance macro_boundary [list $llx $lly $urx $ury]
    dict set instances [$inst getName] $instance

    set_instance_halo [$inst getName] [get_default_halo]
  }

  return $instances
}

proc get_master_pg_pins_area {macro_name} {
  variable macros

  return [dict get $macros $macro_name pins_area]
}

proc get_instance_pg_pins_area {inst_name} {
  variable instances

  set instance [dict get $instances $inst_name]
  set inst [dict get $instance inst]

  set master_area [transform_box {*}[get_master_pg_pins_area [[$inst getMaster] getName]] [$inst getOrigin] [$inst getOrient]]
}

proc set_instance_halo {inst_name halo} {
  variable instances

  set instance [dict get $instances $inst_name]
  set inst [dict get $instance inst]

  if {[$inst getHalo] != "NULL"} {
    set halo [list \
      [[$inst getHalo] xMin] \
      [[$inst getHalo] yMin] \
      [[$inst getHalo] xMax] \
      [[$inst getHalo] yMax] \
      ]
  }
  dict set instances $inst_name halo $halo
  # debug "Inst: [$inst getName], halo: [dict get $instances $inst_name halo]"

  set llx [expr round([dict get $instance xmin] - [lindex $halo 0])]
  set lly [expr round([dict get $instance ymin] - ([lindex $halo 1] - [get_rail_width] / 2))]
  set urx [expr round([dict get $instance xmax] + [lindex $halo 2])]
  set ury [expr round([dict get $instance ymax] + ([lindex $halo 3] - [get_rail_width] / 2))]

  dict set instances $inst_name halo_boundary [list $llx $lly $urx $ury]
}

proc find_instances_of {macro_names} {
  variable design_data
  variable macros

  set selected_instances {}

  dict for {inst_name instance} [get_instances] {
    set macro_name [dict get $instance macro]
    if {[lsearch -exact $macro_names $macro_name] == -1} {continue}
    dict set selected_instances $inst_name $instance
  }

  return $selected_instances
}

proc export_opendb_vias {} {
  variable physical_viarules
  variable block
  variable tech
  # debug "[llength $physical_viarules]"
  dict for {name rules} $physical_viarules {
    foreach rule $rules {
    # Dont create illegal vias
    if {[dict exists $rule illegal]} {continue}
    if {[dict exists $rule fixed]} {continue}

    # debug "$rule"
      set via [$block findVia [dict get $rule name]]
      if {$via == "NULL"} {
        set via [odb::dbVia_create $block [dict get $rule name]]
        # debug "Via $via"

        $via setViaGenerateRule [$tech findViaGenerateRule [dict get $rule rule]]
        set params [$via getViaParams]
        $params setBottomLayer [$tech findLayer [lindex [dict get $rule layers] 0]]
        $params setCutLayer [$tech findLayer [lindex [dict get $rule layers] 1]]
        $params setTopLayer [$tech findLayer [lindex [dict get $rule layers] 2]]
        $params setXCutSize [lindex [dict get $rule cutsize] 0]
        $params setYCutSize [lindex [dict get $rule cutsize] 1]
        $params setXCutSpacing [lindex [dict get $rule cutspacing] 0]
        $params setYCutSpacing [lindex [dict get $rule cutspacing] 1]
        $params setXBottomEnclosure [lindex [dict get $rule enclosure] 0]
        $params setYBottomEnclosure [lindex [dict get $rule enclosure] 1]
        $params setXTopEnclosure [lindex [dict get $rule enclosure] 2]
        $params setYTopEnclosure [lindex [dict get $rule enclosure] 3]
        $params setNumCutRows [lindex [dict get $rule rowcol] 0]
        $params setNumCutCols [lindex [dict get $rule rowcol] 1]

        $via setViaParams $params
      }
    }
  }
  # debug "end"
}

proc get_global_connect_list_default {voltage_domain is_region} {
  variable block
  variable voltage_domains

  foreach net_type "primary_power primary_ground" {
    set net_name [dict get $voltage_domains $voltage_domain $net_type]
    foreach sub_net $net_name {
      set net [$block findNet $sub_net]
      foreach term [get_valid_mterms $sub_net] {
        add_global_connect -net $sub_net -pin_pattern $term
      }
    }
  }
}

proc get_global_connect_list {net_name} {
  variable design_data
  variable global_connections
  variable voltage_domains

  set connect_patterns {}
  if {[dict exist $global_connections $net_name]} {
    foreach pattern [dict get $global_connections $net_name] {
      lappend connect_patterns $pattern
    }
  }

  return $connect_patterns
}

proc export_opendb_global_connection {} {
  variable block
  variable design_data
  variable global_connections
  variable voltage_domains

  ## Do global connect statements first
  get_global_connect_list_default [dict get $design_data core_domain] false

  foreach net_name [get_all_pg_nets] {
    set net [$block findNet $net_name]
    foreach pattern [get_global_connect_list $net_name] {
      add_global_connect -inst_pattern [dict get $pattern inst_name] -pin_pattern [dict get $pattern pin_name] -net $net_name
    }
  }

  ## Do regions second
  set core_domain_name [dict get $design_data core_domain]
  foreach voltage_domain [dict keys $voltage_domains] {
    if {$voltage_domain != $core_domain_name} {
      get_global_connect_list_default $voltage_domain true

      foreach {net_type netname} [dict get $voltage_domains $voltage_domain] {
        set net [$block findNet $net_name]
        # loop over all patterns
        foreach pattern [get_global_connect_list $net_name] {
          add_global_connect -inst_pattern [dict get $pattern inst_name] -pin_pattern [dict get $pattern pin_name] -net $net_name -region $voltage_domain
        }
      }
    }
  }
}

proc export_opendb_specialnet {net_name} {
  variable block
  variable instances
  variable metal_layers
  variable tech
  variable stripe_locs
  variable global_connections
  variable design_data

  set net [$block findNet $net_name]
  if {$net == "NULL"} {
    set net [odb::dbNet_create $block $net_name]
  }
  $net setSpecial
  $net setSigType [get_signal_type $net_name]
  # debug "net $net_name. signaltype, [get_signal_type $net_name], global_connections: $global_connections"

  if {[check_snet_is_unique $net]} {
    $net setWildConnected
  }
  set swire [odb::dbSWire_create $net "ROUTED"]

  # debug "layers - $metal_layers"
  foreach lay $metal_layers {
    if {[array names stripe_locs "$lay,$net_name"] == ""} {continue}

    set layer [find_layer $lay]
    # debug "layer: [$layer getName], num_rects: [llength [odb::getRectangles $stripe_locs($lay,$net_name)]]"
    foreach rect [::odb::getRectangles $stripe_locs($lay,$net_name)] {
      set xMin [$rect xMin]
      set xMax [$rect xMax]
      set yMin [$rect yMin]
      set yMax [$rect yMax]

      set width [expr $xMax - $xMin]
      set height [expr $yMax - $yMin]

      set wire_type "STRIPE"
      if {[is_rails_layer $lay]} {set wire_type "FOLLOWPIN"}
      # debug "  [$layer getName] $xMin $yMin $xMax $yMax $wire_type"
      odb::dbSBox_create $swire $layer $xMin $yMin $xMax $yMax $wire_type
    }
  }

  variable vias
  # debug "vias - [llength $vias]"
  foreach via $vias {
    if {[dict get $via net_name] == $net_name} {
    # For each layer between l1 and l2, add vias at the intersection
      foreach via_inst [dict get $via connections] {
      # debug "$via_inst"
        set via_name [dict get $via_inst name]
        set x        [dict get $via_inst x]
        set y        [dict get $via_inst y]
        # debug "$via_name $x $y [$block findVia $via_name]"
        if {[set defvia [$block findVia $via_name]] != "NULL"} {
        odb::dbSBox_create $swire $defvia $x $y "STRIPE"
      } elseif {[set techvia [$tech findVia $via_name]] != "NULL"} {
        odb::dbSBox_create $swire $techvia $x $y "STRIPE"
      } else {
        utl::error PDN 9069 "Cannot find via $via_name."
      }
    # debug "via created"
    }
  }
}
# debug "end"
}

proc export_opendb_specialnets {} {
  variable block
  variable design_data
  variable voltage_domains
  variable stripe_locs

  foreach net_name [get_all_pg_nets] {
    if {[lsearch -regexp [array names stripe_locs] ",$net_name\$"] > -1} {
      export_opendb_specialnet $net_name
    }
  }

  export_opendb_global_connection
}

proc export_opendb_power_pin {net_name} {
  variable metal_layers
  variable block
  variable stripe_locs
  variable tech
  variable voltage_domains
  variable design_data

  if {![dict exists $design_data grid stdcell]} {return}

  set pins_layers {}
  dict for {grid_name grid} [dict get $design_data grid stdcell] {
    if {[dict exists $grid pins]} {
      lappend pins_layers {*}[dict get $grid pins]
    }
  }
  set pins_layers [lsort -unique $pins_layers]
  if {[llength $pins_layers] == 0} {return}

  set net [$block findNet $net_name]
  if {$net == "NULL"} {
    utl::error PDN 9070 "Cannot find net $net_name in the design."
  }
  set bterms [$net getBTerms]
  if {[llength $bterms] < 1} {
    set bterm [odb::dbBTerm_create $net "${net_name}"]
    if {$bterm == "NULL"} {
      utl::error PDN 9071 "Cannot create terminal for net $net_name."
    }
  }
  # debug $bterm
  foreach bterm [$net getBTerms] {
    $bterm setSigType [get_signal_type $net_name]
  }
  set bterm [lindex [$net getBTerms] 0]
  set bpin [odb::dbBPin_create $bterm]
  $bpin setPlacementStatus "FIRM"

  dict for {domain domain_info} $voltage_domains {
    if {$domain != [dict get $design_data core_domain] &&
        $net_name == [dict get $domain_info primary_power]} {
      set r_pin "r_$net_name"
      set r_net [odb::dbNet_create $block $r_pin]
      set r_bterm [odb::dbBTerm_create $r_net "${r_pin}"]

      set r_bpin [odb::dbBPin_create $r_bterm]
      $r_bpin setPlacementStatus "FIRM"
    }
  }

  foreach lay [lreverse $metal_layers] {
    if {[array names stripe_locs "$lay,$net_name"] == "" ||
        [lsearch -exact $pins_layers $lay] == -1} {continue}
    foreach shape [::odb::getPolygons $stripe_locs($lay,$net_name)] {
      set points [::odb::getPoints $shape]
      if {[llength $points] != 4} {
      # We already issued a message for this - no need to repeat
        continue
      }
      set xMin [expr min([[lindex $points 0] getX], [[lindex $points 1] getX], [[lindex $points 2] getX], [[lindex $points 3] getX])]
      set xMax [expr max([[lindex $points 0] getX], [[lindex $points 1] getX], [[lindex $points 2] getX], [[lindex $points 3] getX])]
      set yMin [expr min([[lindex $points 0] getY], [[lindex $points 1] getY], [[lindex $points 2] getY], [[lindex $points 3] getY])]
      set yMax [expr max([[lindex $points 0] getY], [[lindex $points 1] getY], [[lindex $points 2] getY], [[lindex $points 3] getY])]

      set layer [$tech findLayer $lay]
      odb::dbBox_create $bpin $layer $xMin $yMin $xMax $yMax
      if {[info exists r_bpin]} {
        odb::dbBox_create $r_bpin $layer $xMin $yMin $xMax $yMax
      }

    }
  }
}

proc export_opendb_power_pins {} {
  foreach net_name [get_all_pg_nets] {
    export_opendb_power_pin $net_name
  }
}

## procedure for file existence check, returns 0 if file does not exist or file exists, but empty
proc file_exists_non_empty {filename} {
  return [expr [file exists $filename] && [file size $filename] > 0]
}

proc get {args} {
  variable design_data

  return [dict get $design_data {*}$args]
}
proc get_macro_power_pins {inst_name} {
  set specification [select_instance_specification $inst_name]
  if {[dict exists $specification power_pins]} {
    return [dict get $specification power_pins]
  }
  return "VDDPE VDDCE"
}
proc get_macro_ground_pins {inst_name} {
  set specification [select_instance_specification $inst_name]
  if {[dict exists $specification ground_pins]} {
    return [dict get $specification ground_pins]
  }
  return "VSSE"
}

proc transform_box {xmin ymin xmax ymax origin orientation} {
  switch -exact $orientation {
    R0    {set new_box [list $xmin $ymin $xmax $ymax]}
    R90   {set new_box [list [expr -1 * $ymax] $xmin [expr -1 * $ymin] $xmax]}
    R180  {set new_box [list [expr -1 * $xmax] [expr -1 * $ymax] [expr -1 * $xmin] [expr -1 * $ymin]]}
    R270  {set new_box [list $ymin [expr -1 * $xmax] $ymax [expr -1 * $xmin]]}
    MX    {set new_box [list $xmin [expr -1 * $ymax] $xmax [expr -1 * $ymin]]}
    MY    {set new_box [list [expr -1 * $xmax] $ymin [expr -1 * $xmin] $ymax]}
    MXR90 {set new_box [list $ymin $xmin $ymax $xmax]}
    MYR90 {set new_box [list [expr -1 * $ymax] [expr -1 * $xmax] [expr -1 * $ymin] [expr -1 * $xmin]]}
    default {utl::error PDN 9027 "Illegal orientation $orientation specified."}
  }
  return [list \
    [expr [lindex $new_box 0] + [lindex $origin 0]] \
    [expr [lindex $new_box 1] + [lindex $origin 1]] \
    [expr [lindex $new_box 2] + [lindex $origin 0]] \
    [expr [lindex $new_box 3] + [lindex $origin 1]] \
    ]
}

proc set_template_size {width height} {
  variable template

  dict set template width [ord::microns_to_dbu $width]
  dict set template height [ord::microns_to_dbu $height]
}

proc get_memory_instance_pg_pins {} {
  variable block
  variable metal_layers

  # debug "start"
  set boundary [odb::newSetFromRect {*}[get_core_area]]

  foreach inst [$block getInsts] {
    set inst_name [$inst getName]
    set master [$inst getMaster]

    if {![$master isBlock]} {continue}

    # If there are no shapes left after 'and'ing the boundard with the cell, then
    # the cell lies outside the area where we are adding a power grid.
    set bbox [$inst getBBox]
    set box [odb::newSetFromRect [$bbox xMin] [$bbox yMin] [$bbox xMax] [$bbox yMax]]
    if {[llength [odb::getPolygons [odb::andSet $boundary $box]]] == 0} {
    # debug "Instance [$inst getName] does not lie in the cell area"
      continue
    }

    # debug "cell name - [$master getName]"

    foreach term_name [concat [get_macro_power_pins $inst_name] [get_macro_ground_pins $inst_name]] {
      set master [$inst getMaster]
      if {[set iterm [$inst findITerm $term_name]] == "NULL"} {
        utl::error PDN 9037 "Pin $term_name of instance [$inst getName] is not connected to any net."
      }
      if {[$iterm getNet] == "NULL"} {continue}
      set net_name [[$iterm getNet] getName]
      set mterm [$master findMTerm $term_name]
      if {$mterm == "NULL"} {
        utl::warn PDN 9177 "Cannot find pin $term_name on instance [$inst getName] ([[$inst getMaster] getName])."
        continue
      }

      foreach mPin [$mterm getMPins] {
        foreach geom [$mPin getGeometry] {
          set layer [[$geom getTechLayer] getName]
          if {[lsearch -exact $metal_layers $layer] == -1} {continue}

          set box [transform_box [$geom xMin] [$geom yMin] [$geom xMax] [$geom yMax] [$inst getOrigin] [$inst getOrient]]

          set width  [expr abs([lindex $box 2] - [lindex $box 0])]
          set height [expr abs([lindex $box 3] - [lindex $box 1])]

          if {$width > $height} {
            set layer_name ${layer}_PIN_hor
          } else {
            set layer_name ${layer}_PIN_ver
          }
          # debug "Adding pin for [$inst getName]:[$mterm getName] to layer $layer_name ($box)"
          add_stripe $layer_name $net_name [odb::newSetFromRect {*}$box]
        }
      }
    }
  }
  # debug "end"
}

proc set_core_area {xmin ymin xmax ymax} {
  variable design_data

  dict set design_data config core_area [list $xmin $ymin $xmax $ymax]
}

proc get_core_area {} {
  variable design_data

  return [get_extent [get_stdcell_area]]
}

proc write_pdn_strategy {} {
  variable design_data

  if {[dict exists $design_data grid]} {
    set_pdn_string_property_value "strategy" [dict get $design_data grid]
  }

}

proc init_tech {} {
  variable db
  variable block
  variable tech
  variable libs

  set db [ord::get_db]
  set tech [ord::get_db_tech]
  set libs [$db getLibs]
  set block [ord::get_db_block]

  init_metal_layers
  init_via_tech
}

proc add_power_net {net_names} {
  variable power_nets

  foreach net_name $net_names {
    if {[lsearch -exact $power_nets $net_name] == -1} {
      lappend power_nets $net_name
    }
  }
}

proc add_ground_net {net_names} {
  variable ground_nets

  foreach net_name $net_names {
    if {[lsearch -exact $ground_nets $net_name] == -1} {
      lappend ground_nets $net_name
    }
  }
}

proc get_default_halo {} {
  if {[info vars ::halo] != ""} {
    if {[llength $::halo] == 1} {
      set default_halo "$::halo $::halo $::halo $::halo"
    } elseif {[llength $::halo] == 2} {
      set default_halo "$::halo $::halo"
    } elseif {[llength $::halo] == 4} {
      set default_halo $::halo
    } else {
      utl::error PDN 9029 "Illegal number of elements defined for ::halo \"$::halo\" (1, 2 or 4 allowed)."
    }
  } else {
    set default_halo "0 0 0 0"
  }
  return [lmap x $default_halo {ord::microns_to_dbu $x}]
}

proc get_row_height {} {
  set first_row [lindex [[ord::get_db_block] getRows] 0]
  set row_site [$first_row getSite]

  return [$row_site getHeight]
}

proc init {args} {
  variable db
  variable block
  variable tech
  variable libs
  variable design_data
  variable def_output
  variable default_grid_data
  variable design_name
  variable stripe_locs
  variable site
  variable row_height
  variable metal_layers
  variable stripes_start_with
  variable physical_viarules
  variable stdcell_area
  variable voltage_domains
  variable global_connections
  variable default_global_connections
  variable power_nets
  variable ground_nets

  # debug "start"
  init_tech

  set design_name [$block getName]

  set physical_viarules {}
  set stdcell_area ""

  set die_area [$block getDieArea]

  utl::info PDN 9008 "Design name is $design_name."
  set def_output "${design_name}_pdn.def"

  # debug "examine vars"
  if {$power_nets == {}} {
    if {[info vars ::power_nets] == ""} {
      set power_nets "VDD"
    } else {
      set power_nets $::power_nets
    }
  }

  if {$ground_nets == {}} {
    if {[info vars ::ground_nets] == ""} {
      set ground_nets "VSS"
    } else {
      set ground_nets $::ground_nets
    }
  }

  if {[info vars ::core_domain] == ""} {
    set core_domain "CORE"
    if {![dict exists $voltage_domains $core_domain primary_power]} {
      dict set voltage_domains $core_domain primary_power [lindex $power_nets 0]
    }
    if {![dict exists $voltage_domains $core_domain primary_ground]} {
      dict set voltage_domains $core_domain primary_ground [lindex $ground_nets 0]
    }
  } else {
    set core_domain $::core_domain
  }

  if {[info vars ::stripes_start_with] == ""} {
    set stripes_start_with "GROUND"
  } else {
    set stripes_start_with $::stripes_start_with
  }

  dict set design_data power_nets $power_nets
  dict set design_data ground_nets $ground_nets
  dict set design_data core_domain $core_domain

  # Sourcing user inputs file
  #
  set row_height [get_row_height]

  ##### Get information from BEOL LEF
  utl::info PDN 9009 "Reading technology data."

  if {[info vars ::layers] != ""} {
    foreach layer $::layers {
      if {[dict exists $::layers $layer widthtable]} {
        dict set ::layers $layer widthtable [lmap x [dict get $::layers $layer widthtable] {ord::microns_to_dbu $x}]
      }
    }
    set_layer_info $::layers
  }

  dict set design_data config def_output   $def_output
  dict set design_data config design       $design_name
  dict set design_data config die_area     [list [$die_area xMin]  [$die_area yMin] [$die_area xMax] [$die_area yMax]]

  array unset stripe_locs

  ########################################
  # Remove existing power/ground nets
  #######################################
  foreach pg_net [get_pg_nets] {
    set net [$block findNet $pg_net]
    if {$net != "NULL"} {
      foreach swire [$net getSWires] {
        odb::dbSWire_destroy $swire
      }
    }
  }

  # debug "Set the core area"
  # Set the core area
  if {[info vars ::core_area_llx] != "" && [info vars ::core_area_lly] != "" && [info vars ::core_area_urx] != "" && [info vars ::core_area_ury] != ""} {
  # The core area is larger than the stdcell area by half a rail, since the stdcell rails extend beyond the rails
    set_core_area \
      [ord::microns_to_dbu $::core_area_llx] \
      [ord::microns_to_dbu $::core_area_lly] \
      [ord::microns_to_dbu $::core_area_urx] \
      [ord::microns_to_dbu $::core_area_ury]
  } else {
    set_core_area {*}[get_extent [get_stdcell_plus_area]]
  }

  ##### Basic sanity checks to see if inputs are given correctly
  foreach layer [get_rails_layers] {
    if {[lsearch -exact $metal_layers $layer] < 0} {
      utl::error PDN 9030 "Layer specified for stdcell rails '$layer' not in list of layers."
    }
  }
  # debug "end"

  return $design_data
}

proc convert_layer_spec_to_def_units {data} {
  foreach key {width pitch spacing offset pad_offset core_offset} {
    if {[dict exists $data $key]} {
      dict set data $key [ord::microns_to_dbu [dict get $data $key]]
    }
  }
  return $data
}

proc specify_grid {type specification} {
  if {![dict exists $specification type]} {
    dict set specification type $type
  }
  verify_grid $specification
}

proc get_quadrant {rect x y} {
  set dw [expr [lindex $rect 2] - [lindex $rect 0]]
  set dh [expr [lindex $rect 3] - [lindex $rect 1]]

  set test_x [expr $x - [lindex $rect 0]]
  set test_y [expr $y - [lindex $rect 1]]
  # debug "$dw * $test_y ([expr $dw * $test_y]) > expr $dh * $test_x ([expr $dh * $test_x])"
  if {$dw * $test_y > $dh * $test_x} {
  # Top or left
  if {($dw * $test_y) + ($dh * $test_x) > ($dw * $dh)} {
  # Top or right
    return "t"
  } else {
  # Bottom or left
    return "l"
  }
} else {
# Bottom or right
if {($dw * $test_y) + ($dh * $test_x) > ($dw * $dh)} {
# Top or right
  return "r"
} else {
# Bottom or left
  return "b"
}
}
}

proc get_design_quadrant {x y} {
  variable design_data

  set die_area [dict get $design_data config die_area]
  return [get_quadrant $die_area $x $y]
}

proc get_core_facing_pins {instance pin_name side layer} {
  variable block
  set geoms {}
  set core_pins {}
  set inst [$block findInst [dict get $instance name]]
  if {[set iterm [$inst findITerm $pin_name]] == "NULL"} {
    utl::warn PDN 9055 "Cannot find pin $pin_name on inst [$inst getName]."
    return {}
  }
  if {[set mterm [$iterm getMTerm]] == "NULL"} {
    utl::warn PDN 9056 "Cannot find master pin $pin_name for cell [[$inst getMaster] getName]."
    return {}
  }
  set pins [$mterm getMPins]

  # debug "start"
  foreach pin $pins {
    foreach geom [$pin getGeometry] {
      if {[[$geom getTechLayer] getName] != $layer} {continue}
      lappend geoms $geom
    }
  }
  # debug "$pins"
  foreach geom $geoms {
    set ipin [transform_box [$geom xMin] [$geom yMin] [$geom xMax] [$geom yMax] [$inst getOrigin] [$inst getOrient]]
    # debug "$ipin [[$inst getBBox] xMin] [[$inst getBBox] yMin] [[$inst getBBox] xMax] [[$inst getBBox] yMax] "
    switch $side {
      "t" {
        if {[lindex $ipin 1] == [[$inst getBBox] yMin]} {
          lappend core_pins [list \
            centre [expr ([lindex $ipin 2] + [lindex $ipin 0]) / 2] \
            width [expr [lindex $ipin 2] - [lindex $ipin 0]] \
            ]
        }
      }
      "b" {
        if {[lindex $ipin 3] == [[$inst getBBox] yMax]} {
          lappend core_pins [list \
            centre [expr ([lindex $ipin 2] + [lindex $ipin 0]) / 2] \
            width [expr [lindex $ipin 2] - [lindex $ipin 0]] \
            ]
        }
      }
      "l" {
        if {[lindex $ipin 2] == [[$inst getBBox] xMax]} {
          lappend core_pins [list \
            centre [expr ([lindex $ipin 3] + [lindex $ipin 1]) / 2] \
            width [expr [lindex $ipin 3] - [lindex $ipin 1]] \
            ]
        }
      }
      "r" {
        if {[lindex $ipin 0] == [[$inst getBBox] xMin]} {
          lappend core_pins [list \
            centre [expr ([lindex $ipin 3] + [lindex $ipin 1]) / 2] \
            width [expr [lindex $ipin 3] - [lindex $ipin 1]] \
            ]
        }
      }
    }
  }
  # debug "$core_pins"
  return $core_pins
}

proc connect_pads_to_core_ring {type pin_name pads} {
  variable grid_data
  variable pad_cell_blockages

  dict for {inst_name instance} [find_instances_of $pads] {
    set side [get_design_quadrant [dict get $instance x] [dict get $instance y]]
    switch $side {
      "t" {
        set required_direction "ver"
      }
      "b" {
        set required_direction "ver"
      }
      "l" {
        set required_direction "hor"
      }
      "r" {
        set required_direction "hor"
      }
    }
    foreach non_pref_layer [dict keys [dict get $grid_data core_ring]] {
      if {[get_dir $non_pref_layer] != $required_direction} {
        set non_pref_layer_info [dict get $grid_data core_ring $non_pref_layer]
        break
      }
    }
    # debug "find_layer"
    foreach pref_layer [dict keys [dict get $grid_data core_ring]] {
      if {[get_dir $pref_layer] == $required_direction} {
        break
      }
    }
    switch $side {
      "t" {
        set y_min [expr [get_core_ring_centre $type $side $non_pref_layer_info] - [dict get $grid_data core_ring $non_pref_layer width] / 2]
        set y_min_blk [expr $y_min - [dict get $grid_data core_ring $non_pref_layer spacing]]
        set y_max [dict get $instance ymin]
        # debug "t: [dict get $instance xmin] $y_min_blk [dict get $instance xmax] [dict get $instance ymax]"
        add_padcell_blockage $pref_layer [odb::newSetFromRect [dict get $instance xmin] $y_min_blk [dict get $instance xmax] [dict get $instance ymax]]
      }
      "b" {
      # debug "[get_core_ring_centre $type $side $non_pref_layer_info] + [dict get $grid_data core_ring $non_pref_layer width] / 2"
        set y_max [expr [get_core_ring_centre $type $side $non_pref_layer_info] + [dict get $grid_data core_ring $non_pref_layer width] / 2]
        set y_max_blk [expr $y_max + [dict get $grid_data core_ring $non_pref_layer spacing]]
        set y_min [dict get $instance ymax]
        # debug "b: [dict get $instance xmin] [dict get $instance ymin] [dict get $instance xmax] $y_max"
        add_padcell_blockage $pref_layer [odb::newSetFromRect [dict get $instance xmin] [dict get $instance ymin] [dict get $instance xmax] $y_max_blk]
        # debug "end b"
      }
      "l" {
        set x_max [expr [get_core_ring_centre $type $side $non_pref_layer_info] + [dict get $grid_data core_ring $non_pref_layer width] / 2]
        set x_max_blk [expr $x_max + [dict get $grid_data core_ring $non_pref_layer spacing]]
        set x_min [dict get $instance xmax]
        # debug "l: [dict get $instance xmin] [dict get $instance ymin] $x_max [dict get $instance ymax]"
        add_padcell_blockage $pref_layer [odb::newSetFromRect [dict get $instance xmin] [dict get $instance ymin] $x_max_blk [dict get $instance ymax]]
      }
      "r" {
        set x_min [expr [get_core_ring_centre $type $side $non_pref_layer_info] - [dict get $grid_data core_ring $non_pref_layer width] / 2]
        set x_min_blk [expr $x_min - [dict get $grid_data core_ring $non_pref_layer spacing]]
        set x_max [dict get $instance xmin]
        # debug "r: $x_min_blk [dict get $instance ymin] [dict get $instance xmax] [dict get $instance ymax]"
        add_padcell_blockage $pref_layer [odb::newSetFromRect $x_min_blk [dict get $instance ymin] [dict get $instance xmax] [dict get $instance ymax]]
      }
    }

    # debug "$pref_layer"
    foreach pin_geometry [get_core_facing_pins $instance $pin_name $side $pref_layer] {
      set centre [dict get $pin_geometry centre]
      set width  [dict get $pin_geometry width]

      variable tech
      if {[[set layer [$tech findLayer $pref_layer]] getMaxWidth] != "NULL" && $width > [$layer getMaxWidth]} {
        set width [$layer getMaxWidth]
      }
      if {$required_direction == "hor"} {
      # debug "added_strap $pref_layer $type $x_min [expr $centre - $width / 2] $x_max [expr $centre + $width / 2]"
        add_stripe $pref_layer "PAD_$type" [odb::newSetFromRect $x_min [expr $centre - $width / 2] $x_max [expr $centre + $width / 2]]
      } else {
      # debug "added_strap $pref_layer $type [expr $centre - $width / 2] $y_min [expr $centre + $width / 2] $y_max"
        add_stripe $pref_layer "PAD_$type" [odb::newSetFromRect [expr $centre - $width / 2] $y_min [expr $centre + $width / 2] $y_max]
      }
    }
  }
  # debug "end"
}

proc add_pad_straps {tag} {
  variable stripe_locs

  foreach pad_connection [array names stripe_locs "*,PAD_*"] {
    if {![regexp "(.*),PAD_$tag" $pad_connection - layer]} {continue}
    # debug "$pad_connection"
    if {[array names stripe_locs "$layer,$tag"] != ""} {
    # debug add_pad_straps "Before: $layer [llength [::odb::getPolygons $stripe_locs($layer,$tag)]]"
    # debug add_pad_straps "Adding: [llength [::odb::getPolygons $stripe_locs($pad_connection)]]"
      add_stripe $layer $tag $stripe_locs($pad_connection)
      # debug add_pad_straps "After:  $layer [llength [::odb::getPolygons $stripe_locs($layer,$tag)]]"
    }
  }
}

proc print_spacing_table {layer_name} {
  set layer [find_layer $layer_name]
  if {[$layer hasTwoWidthsSpacingRules]} {
    set table_size [$layer getTwoWidthsSpacingTableNumWidths]
    for {set i 0} {$i < $table_size} {incr i} {
      set width [$layer getTwoWidthsSpacingTableWidth $i]
      set report_width "WIDTH $width"
      if {[$layer getTwoWidthsSpacingTableHasPRL $i]} {
        set prl [$layer getTwoWidthsSpacingTablePRL $i]
        set report_prl " PRL $prl"
      } else {
        set report_prl ""
      }
      set report_spacing " [$layer getTwoWidthsSpacingTableEntry 0 $i] "
    }
    utl::report "${report_width}${report_prl}${report_spacing}"
  }
}

proc get_twowidths_table {table_type} {
  variable metal_layers
  set twowidths_table {}

  foreach layer_name $metal_layers {
    set spacing_table [get_spacingtables $layer_name]
    set prls {}

    if {[dict exists $spacing_table TWOWIDTHS $table_type]} {
      set layer_spacing_table [dict get $spacing_table TWOWIDTHS $table_type]
      set table_size [dict size $layer_spacing_table]
      set table_widths [dict keys $layer_spacing_table]

      for {set i 0} {$i < $table_size} {incr i} {

        set width [lindex $table_widths $i]
        set spacing [lindex [dict get $layer_spacing_table $width spacings] $i]

        if {[dict get $layer_spacing_table $width prl] != 0} {
          set prl [dict get $layer_spacing_table $width prl]
          set update_prls {}
          dict for {prl_entry prl_setting} $prls {
            if {$prl <= [lindex $prl_entry 0]} {break}
            dict set update_prls $prl_entry $prl_setting
            dict set twowidths_table $layer_name $width $prl_entry $prl_setting
          }
          dict set update_prls $prl $spacing
          dict set twowidths_table $layer_name $width $prl $spacing
          set prls $update_prls
        } else {
          set prls {}
          dict set prls 0 $spacing
          dict set twowidths_table $layer_name $width 0 $spacing
        }
      }
    }
  }

  return $twowidths_table
}

proc get_twowidths_tables {} {
  variable twowidths_table
  variable twowidths_table_wrongdirection

  set twowidths_table [get_twowidths_table NONE]
  set twowidths_table_wrongdirection [get_twowidths_table WRONGDIRECTION]
}

proc select_from_table {table width} {
  foreach value [lreverse [lsort -integer [dict keys $table]]] {
    if {$width > $value} {
      return $value
    }
  }
  return [lindex [dict keys $table] 0]
}

proc get_preferred_direction_spacing {layer_name width prl} {
  variable twowidths_table

  # debug "$layer_name $width $prl"
  # debug "twowidths_table $twowidths_table"
  if {$twowidths_table == {}} {
    return [[find_layer $layer_name] getSpacing]
  } else {
    set width_key [select_from_table [dict get $twowidths_table $layer_name] $width]
    set prl_key   [select_from_table [dict get $twowidths_table $layer_name $width_key] $prl]
  }

  return [dict get $twowidths_table $layer_name $width_key $prl_key]
}

proc get_nonpreferred_direction_spacing {layer_name width prl} {
  variable twowidths_table_wrongdirection

  # debug "twowidths_table_wrong_direction $twowidths_table_wrongdirection"
  if {[dict exists $twowidths_table_wrongdirection $layer_name]} {
    set width_key [select_from_table [dict get $twowidths_table_wrongdirection $layer_name] $width]
    set prl_key   [select_from_table [dict get $twowidths_table_wrongdirection $layer_name $width_key] $prl]
  } else {
    return [get_preferred_direction_spacing $layer_name $width $prl]
  }

  return [dict get $twowidths_table_wrongdirection $layer_name $width_key $prl_key]
}

proc create_obstructions {layer_name polygons} {
  set layer [find_layer $layer_name]
  set min_spacing [get_preferred_direction_spacing $layer_name 0 0]

  # debug "Num polygons [llength $polygons]"

  foreach polygon $polygons {
    set points [::odb::getPoints $polygon]
    if {[llength $points] != 4} {
      utl::warn PDN 9006 "Unexpected number of points in stripe of $layer_name."
      continue
    }
    set xMin [expr min([[lindex $points 0] getX], [[lindex $points 1] getX], [[lindex $points 2] getX], [[lindex $points 3] getX])]
    set xMax [expr max([[lindex $points 0] getX], [[lindex $points 1] getX], [[lindex $points 2] getX], [[lindex $points 3] getX])]
    set yMin [expr min([[lindex $points 0] getY], [[lindex $points 1] getY], [[lindex $points 2] getY], [[lindex $points 3] getY])]
    set yMax [expr max([[lindex $points 0] getY], [[lindex $points 1] getY], [[lindex $points 2] getY], [[lindex $points 3] getY])]

    if {[get_dir $layer_name] == "hor"} {
      set required_spacing_pref    [get_preferred_direction_spacing $layer_name [expr $yMax - $yMin] [expr $xMax - $xMin]]
      set required_spacing_nonpref [get_nonpreferred_direction_spacing $layer_name [expr $xMax - $xMin] [expr $yMax - $yMin]]

      set y_change [expr $required_spacing_pref    - $min_spacing]
      set x_change [expr $required_spacing_nonpref - $min_spacing]
    } else {
      set required_spacing_pref    [get_preferred_direction_spacing $layer_name [expr $xMax - $xMin] [expr $yMax - $yMin]]
      set required_spacing_nonpref [get_nonpreferred_direction_spacing $layer_name [expr $yMax - $yMin] [expr $xMax - $xMin]]

      set x_change [expr $required_spacing_pref    - $min_spacing]
      set y_change [expr $required_spacing_nonpref - $min_spacing]
    }

    create_obstruction_object_blockage $layer $min_spacing [expr $xMin - $x_change] [expr $yMin - $y_change] [expr $xMax + $x_change] [expr $yMax + $y_change]
  }
}

proc combine {lside rside} {
# debug "l [llength $lside] r [llength $rside]"
if {[llength $lside] > 1} {
  set lside [combine [lrange $lside 0 [expr [llength $lside] / 2 - 1]] [lrange $lside [expr [llength $lside] / 2] end]]
}
if {[llength $rside] > 1} {
  set rside [combine [lrange $rside 0 [expr [llength $rside] / 2 - 1]] [lrange $rside [expr [llength $rside] / 2] end]]
}
return [odb::orSet $lside $rside]
}

proc shapes_to_polygonSet {shapes} {
  if {[llength $shapes] == 1} {
    return $shapes
  }
  return [combine [lrange $shapes 0 [expr [llength $shapes] / 2 - 1]] [lrange $shapes [expr [llength $shapes] / 2] end]]
}

proc generate_obstructions {layer_name} {
  variable stripe_locs

  # debug "layer $layer_name"
  get_twowidths_tables

  set block_shapes {}
  foreach net [get_pg_nets] {
    if {[array names stripe_locs $layer_name,$net] == ""} {
    # debug "No polygons on $layer_name,$net"
      continue
    }
    if {$block_shapes == {}} {
      set block_shapes $stripe_locs($layer_name,$net)
    } else {
      set block_shapes [odb::orSet $block_shapes $stripe_locs($layer_name,$net)]
    }
  }
  set via_shapes 0
  variable vias
  # debug "vias - [llength $vias]"
  foreach via $vias {
  # For each layer between l1 and l2, add vias at the intersection
  foreach via_inst [dict get $via connections] {
  # debug "$via_inst"
    set via_name [dict get $via_inst name]
    set x        [dict get $via_inst x]
    set y        [dict get $via_inst y]

    set lower_layer_name [lindex [dict get $via_inst layers] 0]
    set upper_layer_name [lindex [dict get $via_inst layers] 2]

    if {$lower_layer_name == $layer_name && [dict exists $via_inst lower_rect]} {
      lappend block_shapes [odb::newSetFromRect {*}[transform_box {*}[dict get $via_inst lower_rect] [list $x $y] "R0"]]
      incr via_shapes
    } elseif {$upper_layer_name == $layer_name && [dict exists $via_inst upper_rect]} {
      lappend block_shapes [odb::newSetFromRect {*}[transform_box {*}[dict get $via_inst upper_rect] [list $x $y] "R0"]]
      incr via_shapes
    }
  }
}
# debug "Via shapes $layer_name $via_shapes"
if {$block_shapes != {}} {
# debug "create_obstructions [llength $block_shapes]"
create_obstructions $layer_name [odb::getPolygons [shapes_to_polygonSet $block_shapes]]
  }
  # debug "end"
}

proc create_obstruction_object_blockage {layer min_spacing xMin yMin xMax yMax} {
  variable block


  set layer_pitch [get_pitch $layer]
  set layer_width [$layer getWidth]
  # debug "Layer - [$layer getName], pitch $layer_pitch, width $layer_width"
  set tracks [$block findTrackGrid $layer]
  set offsetX [lindex [$tracks getGridX] 0]
  set offsetY [lindex [$tracks getGridY] 0]

  # debug "OBS: [$layer getName] $xMin $yMin $xMax $yMax (dx [expr $xMax - $xMin] dy [expr $yMax - $yMin])"
  # debug "Offsets: x $offsetX y $offsetY"
  set relative_xMin [expr $xMin - $offsetX]
  set relative_xMax [expr $xMax - $offsetX]
  set relative_yMin [expr $yMin - $offsetY]
  set relative_yMax [expr $yMax - $offsetY]
  # debug "relative to core area $relative_xMin $relative_yMin $relative_xMax $relative_yMax"

  # debug "OBS: [$layer getName] $xMin $yMin $xMax $yMax"
  # Determine which tracks are blocked
  if {[get_dir [$layer getName]] == "hor"} {
    set pitch_start [expr $relative_yMin / $layer_pitch]
    if {$relative_yMin % $layer_pitch >= ($min_spacing + $layer_width / 2)} {
      incr pitch_start
    }
    set pitch_end [expr $relative_yMax / $layer_pitch]
    if {$relative_yMax % $layer_pitch > $layer_width / 2} {
      incr pitch_end
    }
    # debug "pitch: start $pitch_start end $pitch_end"
    for {set i $pitch_start} {$i <= $pitch_end} {incr i} {
      set obs [odb::dbObstruction_create $block $layer \
        $xMin \
        [expr $i * $layer_pitch + $offsetY - $layer_width / 2] \
        $xMax \
        [expr $i * $layer_pitch + $offsetY + $layer_width / 2] \
      ]
    }
  } else {
    set pitch_start [expr $relative_xMin / $layer_pitch]
    if {$relative_xMin % $layer_pitch >= ($min_spacing + $layer_width / 2)} {
      incr pitch_start
    }
    set pitch_end [expr $relative_xMax / $layer_pitch]
    if {$relative_xMax % $layer_pitch > $layer_width / 2} {
      incr pitch_end
    }
    # debug "pitch: start $pitch_start end $pitch_end"
    for {set i $pitch_start} {$i <= $pitch_end} {incr i} {
      set obs [odb::dbObstruction_create $block $layer \
        [expr $i * $layer_pitch + $offsetX - $layer_width / 2] \
        $yMin \
        [expr $i * $layer_pitch + $offsetX + $layer_width / 2] \
        $yMax \
      ]
    }
  }
}

proc create_obstruction_object_net {layer min_spacing xMin yMin xMax yMax} {
  variable block
  variable obstruction_index

  incr obstruction_index
  set net_name "obstruction_$obstruction_index"
  if {[set obs_net [$block findNet $net_name]] == "NULL"} {
    set obs_net [odb::dbNet_create $block $net_name]
  }
  # debug "obs_net [$obs_net getName]"
  if {[set wire [$obs_net getWire]] == "NULL"} {
    set wire [odb::dbWire_create $obs_net]
  }
  # debug "Wire - net [[$wire getNet] getName]"
  set encoder [odb::dbWireEncoder]
  $encoder begin $wire

  set layer_pitch [$layer getPitch]
  set layer_width [$layer getWidth]
  # debug "Layer - [$layer getName], pitch $layer_pitch, width $layer_width"
  set core_area [get_core_area]
  # debug "core_area $core_area"
  set relative_xMin [expr $xMin - [lindex $core_area 0]]
  set relative_xMax [expr $xMax - [lindex $core_area 0]]
  set relative_yMin [expr $yMin - [lindex $core_area 1]]
  set relative_yMax [expr $yMax - [lindex $core_area 1]]
  # debug "relative to core area $relative_xMin $relative_yMin $relative_xMax $relative_yMax"

  # debug "OBS: [$layer getName] $xMin $yMin $xMax $yMax"
  # Determine which tracks are blocked
  if {[get_dir [$layer getName]] == "hor"} {
    set pitch_start [expr $relative_yMin / $layer_pitch]
    if {$relative_yMin % $layer_pitch > ($min_spacing + $layer_width / 2)} {
      incr pitch_start
    }
    set pitch_end [expr $relative_yMax / $layer_pitch]
    if {$relative_yMax % $layer_pitch > $layer_width / 2} {
      incr pitch_end
    }
    for {set i $pitch_start} {$i <= $pitch_end} {incr i} {
      $encoder newPath $layer ROUTED
      $encoder addPoint [expr $relative_xMin + [lindex $core_area 0]] [expr $i * $layer_pitch + [lindex $core_area 1]]
      $encoder addPoint [expr $relative_xMax + [lindex $core_area 0]] [expr $i * $layer_pitch + [lindex $core_area 1]]
    }
  } else {
    set pitch_start [expr $relative_xMin / $layer_pitch]
    if {$relative_xMin % $layer_pitch > ($min_spacing + $layer_width / 2)} {
      incr pitch_start
    }
    set pitch_end [expr $relative_xMax / $layer_pitch]
    if {$relative_xMax % $layer_pitch > $layer_width / 2} {
      incr pitch_end
    }
    for {set i $pitch_start} {$i <= $pitch_end} {incr i} {
      $encoder newPath $layer ROUTED
      $encoder addPoint [expr $i * $layer_pitch + [lindex $core_area 0]] [expr $relative_yMin + [lindex $core_area 1]]
      $encoder addPoint [expr $i * $layer_pitch + [lindex $core_area 0]] [expr $relative_yMax + [lindex $core_area 1]]
    }
  }
  $encoder end
}

proc add_grid {} {
  variable design_data
  variable grid_data

  if {[dict exists $grid_data core_ring]} {
    set area [dict get $grid_data area]
    # debug "Area $area"

    generate_core_rings [dict get $grid_data core_ring]

    if {[dict exists $grid_data gnd_pads]} {
      dict for {pin_name cells} [dict get $grid_data gnd_pads] {
        connect_pads_to_core_ring "GROUND" $pin_name $cells
      }
    }
    if {[dict exists $grid_data pwr_pads]} {
      dict for {pin_name cells} [dict get $grid_data pwr_pads] {
        connect_pads_to_core_ring "POWER" $pin_name $cells
      }
    }

    # generate_voltage_domain_rings [dict get $grid_data core_ring]
    # merge_stripes
    # set intersections [odb::andSet $stripe_locs(G1,POWER) $stripe_locs(G2,POWER)]
    # debug "# intersections [llength [odb::getPolygons $intersections]]"
    # foreach pwr_net [get_power_nets] {
    #   generate_grid_vias $pwr_net
    # }
    # foreach gnd_net [get_ground_nets] {
    #   generate_grid_vias $gnd_net
    # }
    apply_padcell_blockages
  }

  set area [dict get $grid_data area]

  if {[dict exists $grid_data rails]} {
    # debug "Adding stdcell rails"
    # debug "grid_data: $grid_data"
    set area [dict get $grid_data area]
    # debug "Area $area"
    generate_lower_metal_followpin_rails
  }

  ## Generate stripes on power and ground nets
  foreach net [get_all_pg_nets] {
    # debug "generate stripes for net $net"
    generate_stripes $net
  }
  merge_stripes

  if {[get_voltage_domain_switched_power [dict get $design_data core_domain]] != ""} {
    insert_power_switches
    connect_power_switches
    detect_and_fix_tapcell_overlaps
  }
  
  ## Cut blocked areas on power and ground nets
  foreach net [get_pg_nets] {
    cut_blocked_areas $net
    add_pad_straps $net
  }
  merge_stripes

  if {[dict exists $grid_data obstructions]} {
    utl::info PDN 9032 "Generating blockages for TritonRoute."
    # debug "Obstructions: [dict get $grid_data obstructions]"
    foreach layer_name [dict get $grid_data obstructions] {
      generate_obstructions $layer_name
    }
  }
  # debug "end"
}

proc select_instance_specification {instance} {
  variable design_data
  variable instances

  if {![dict exists $instances $instance grid]} {
    utl::error PDN 9248 "Instance $instance is not associated with any grid"
  }
  return [dict get $design_data grid macro [dict get $instances $instance grid]]
}

proc get_instance_specification {instance} {
  variable instances

  set specification [select_instance_specification $instance]

  if {![dict exists $specification blockages]} {
    dict set specification blockages {}
  }
  dict set specification area [dict get $instances $instance macro_boundary]

  return $specification
}

proc get_pitch {layer} {
  if {[$layer hasXYPitch]} {
    if {[get_dir [$layer getName]] == "hor"} {
      return [$layer getPitchY]
    } else {
      return [$layer getPitchX]
    }
  } else {
    return [$layer getPitch]
  }
}

proc get_layer_number {layer_name} {
  set layer [[ord::get_db_tech] findLayer $layer_name]
  if {$layer == "NULL"} {
    utl::error PDN 9160 "Cannot find layer $layer_name."
  }
  return [$layer getNumber]
}

proc init_metal_layers {} {
  variable metal_layers
  variable layers
  variable block

  set tech [ord::get_db_tech]
  set block [ord::get_db_block]
  set metal_layers {}

  foreach layer [$tech getLayers] {
    if {[$layer getType] == "ROUTING"} {
      set_prop_lines $layer LEF58_TYPE
      # Layers that have LEF58_TYPE are not normal ROUTING layers, so should not be considered
      if {![empty_propline]} {continue}

      set layer_name [$layer getName]
      lappend metal_layers $layer_name

      # debug "Direction ($layer_name): [$layer getDirection]"
      if {[$layer getDirection] == "HORIZONTAL"} {
        dict set layers $layer_name direction "hor"
      } else {
        dict set layers $layer_name direction "ver"
      }
      dict set layers $layer_name pitch [get_pitch $layer]

      set tracks [$block findTrackGrid $layer]
      if {$tracks == "NULL"} {
        utl::warn PDN 9035 "No track information found for layer $layer_name."
      } else {
        dict set layers $layer_name offsetX [lindex [$tracks getGridX] 0]
        dict set layers $layer_name offsetY [lindex [$tracks getGridY] 0]
      }
    }
  }
}

proc get_instance_llx {instance} {
  variable instances
  return [lindex [dict get $instances $instance halo_boundary] 0]
}

proc get_instance_lly {instance} {
  variable instances
  return [lindex [dict get $instances $instance halo_boundary] 1]
}

proc get_instance_urx {instance} {
  variable instances
  return [lindex [dict get $instances $instance halo_boundary] 2]
}

proc get_instance_ury {instance} {
  variable instances
  return [lindex [dict get $instances $instance halo_boundary] 3]
}

proc get_macro_blockage_layers {instance} {
  variable metal_layers

  set specification [select_instance_specification $instance]
  if {[dict exists $specification blockages]} {
    # debug "Block [dict get $specification blockages] for $instance"
    return [dict get $specification blockages]
  }
  return $metal_layers
}

proc report_layer_details {layer} {
  set str " - "
  foreach element {width pitch spacing offset pad_offset core_offset} {
    if {[dict exists $layer $element]} {
      set str [format "$str $element: %.3f " [ord::dbu_to_microns [dict get $layer $element]]]
    }
  }
  return $str
}

proc print_strategy {type specification} {
  if {[dict exists $specification name]} {
    set header "Type: ${type}, [dict get $specification name]"
  } else {
    set header "Type: $type"
  }

  if {$type == "macro"} {
    if {[dict exists $specification grid_over_pg_pins]} {
      if {[dict get $specification grid_over_pg_pins] == 1} {
        set header "$header -grid_over_pg_pins"
      } else {
        set header "$header -grid_over_boundary"
      }
    }
  }

  utl::report $header

  if {[dict exists $specification core_ring]} {
    utl::report "    Core Rings"
    dict for {layer_name layer} [dict get $specification core_ring] {
      set str "      Layer: $layer_name"
      if {[dict exists $layer width]} {
        set str "$str [report_layer_details $layer]"
        utl::report $str
      } else {
        utl::report $str
        foreach template [dict keys $layer] {
          utl::report -nonewline [format "          %-14s %s" $template [report_layer_details [dict get $layer $template]]]
        }
      }
    }
  }
  if {[dict exists $specification rails]} {
    utl::report "    Stdcell Rails"
    dict for {layer_name layer} [dict get $specification rails] {
      if {[dict exists $layer width]} {
        utl::report "      Layer: $layer_name [report_layer_details $layer]"
      } else {
        utl::report "      Layer: $layer_name"
        foreach template [dict keys $layer] {
          utl::report [format "          %-14s %s" $template [report_layer_details [dict get $layer $template]]]
        }
      }
    }
  }
  if {[dict exists $specification instance]} {
    utl::report "    Instance: [dict get $specification instance]"
  }
  if {[dict exists $specification macro]} {
    utl::report "    Macro: [dict get $specification macro]"
  }
  if {[dict exists $specification orient]} {
    utl::report "    Macro orientation: [dict get $specification orient]"
  }
  if {[dict exists $specification straps]} {
    utl::report "    Straps"
    dict for {layer_name layer} [dict get $specification straps] {
      if {[dict exists $layer width]} {
        utl::report "      Layer: $layer_name [report_layer_details $layer]"
      } else {
        utl::report "      Layer: $layer_name"
        foreach template [dict keys $layer] {
          utl::report [format "          %-14s %s" $template [report_layer_details [dict get $layer $template]]]
        }
      }
    }
  }
  if {[dict exists $specification connect]} {
    utl::report "    Connect: [dict get $specification connect]"
  }
  if {[dict exists $specification switch_cell]} {
    utl::report "    Switch cell: [dict get $specification switch_cell]"
  }
}

proc read_template_placement {} {
  variable plan_template
  variable prop_line

  if {![is_defined_pdn_property "plan_template"]} {
    define_template_grid
  } else {
    set plan_template {}
    set prop_line [get_pdn_string_property_value "plan_template"]
    while {![empty_propline]} {
      set line [read_propline]
      if {[llength $line] == 0} {continue}
      set x  [ord::microns_to_dbu [lindex $line 0]]
      set y  [ord::microns_to_dbu [lindex $line 1]]
      set x1 [ord::microns_to_dbu [lindex $line 2]]
      set y1 [ord::microns_to_dbu [lindex $line 3]]
      set template [lindex $line end]

      dict set plan_template $x $y $template
    }
  }
}

proc is_defined_pdn_property {name} {
  variable block

  if {[set pdn_props [::odb::dbBoolProperty_find $block PDN]] == "NULL"} {
    return 0
  }

  if {[::odb::dbStringProperty_find $pdn_props $name] == "NULL"} {
    return 0
  }
  return 1
}

proc get_pdn_string_property {name} {
  variable block

  if {[set pdn_props [::odb::dbBoolProperty_find $block PDN]] == "NULL"} {
    set pdn_props [::odb::dbBoolProperty_create $block PDN 1]
  }

  if {[set prop [::odb::dbStringProperty_find $pdn_props $name]] == "NULL"} {
    set prop [::odb::dbStringProperty_create $pdn_props $name ""]
  }

  return $prop
}

proc set_pdn_string_property_value {name value} {
  set prop [get_pdn_string_property $name]
  $prop setValue $value
}

proc get_pdn_string_property_value {name} {
  set prop [get_pdn_string_property $name]

  return [$prop getValue]
}

proc write_template_placement {} {
  variable plan_template
  variable template

  set str ""
  foreach x [lsort -integer [dict keys $plan_template]] {
    foreach y [lsort -integer [dict keys [dict get $plan_template $x]]] {
      set str [format "${str}%.3f %.3f %.3f %.3f %s ;\n" \
        [ord::dbu_to_microns $x] [ord::dbu_to_microns $y] \
        [ord::dbu_to_microns ($x + [dict get $template width])] [ord::dbu_to_microns ($y + [dict get $template height])] \
        [dict get $plan_template $x $y]
      ]
    }
  }

  set_pdn_string_property_value "plan_template" $str
}

proc get_extent {polygon_set} {
  set first_point  [lindex [odb::getPoints [lindex [odb::getPolygons $polygon_set] 0]] 0]
  set minX [set maxX [$first_point getX]]
  set minY [set maxY [$first_point getY]]

  foreach shape [odb::getPolygons $polygon_set] {
    foreach point [odb::getPoints $shape] {
      set x [$point getX]
      set y [$point getY]
      set minX [expr min($minX,$x)]
      set maxX [expr max($maxX,$x)]
      set minY [expr min($minY,$y)]
      set maxY [expr max($maxY,$y)]
    }
  }

  return [list $minX $minY $maxX $maxY]
}

proc round_to_routing_grid {layer_name location} {
  variable tech
  variable block

  set grid [$block findTrackGrid [$tech findLayer $layer_name]]

  if {[get_dir $layer_name] == "hor"} {
    set grid_points [$grid getGridY]
  } else {
    set grid_points [$grid getGridX]
  }

  set size [llength $grid_points]
  set pos [expr ($size + 1) / 2]

  if {[lsearch -exact $grid_points $location] != -1} {
    return $location
  }
  set prev_pos -1
  set size [expr ($size + 1) / 2]
  while {!(([lindex $grid_points $pos] < $location) && ($location < [lindex $grid_points [expr $pos + 1]]))} {
    if {$prev_pos == $pos} {utl::error PDN 9051 "Infinite loop detected trying to round to grid."}
    set prev_pos $pos
    set size [expr ($size + 1) / 2]

    if {$location > [lindex $grid_points $pos]} {
      set pos [expr $pos + $size]
    } else {
      set pos [expr $pos - $size]
    }
    # debug "[lindex $grid_points $pos] < $location < [lindex $grid_points [expr $pos + 1]]"
    # debug [expr (([lindex $grid_points $pos] < $location) && ($location < [lindex $grid_points [expr $pos + 1]]))]
  }

  return [lindex $grid_points $pos]
}

proc identify_channels {lower_layer_name upper_layer_name net} {
  variable block
  variable stripe_locs
  variable design_data

  set channels {}
  set wire_groups {}

  if {[array names stripe_locs "$lower_layer_name,$net"] == ""} {return $channels}

  set upper_pitch_check [expr round(1.1 * [get_grid_wire_pitch $upper_layer_name])]
  set lower_pitch_check [expr round(1.1 * [get_grid_wire_pitch $lower_layer_name])]
  # debug "stripes $lower_layer_name, net: $net, $stripe_locs($lower_layer_name,$net)"
  # debug "Direction (lower-$lower_layer_name): [get_dir $lower_layer_name] (upper-$upper_layer_name): [get_dir $upper_layer_name]"
  # debug "Pitch check (lower): [ord::dbu_to_microns $lower_pitch_check], (upper): [ord::dbu_to_microns $upper_pitch_check]"
  if {[get_dir $lower_layer_name] ==  "hor"} {
    set channel_wires [odb::subtractSet $stripe_locs($lower_layer_name,$net) [odb::bloatSet [odb::shrinkSet $stripe_locs($lower_layer_name,$net) $upper_pitch_check 0] $upper_pitch_check 0]]
    # Group wires with same xMin and xMax so that the channels form rectangles
    foreach wire [odb::getRectangles $channel_wires] {
      set xMin [$wire xMin]
      set xMax [$wire xMax]
      dict lappend wire_groups "$xMin,$xMax" [odb::newSetFromRect [$wire xMin] [$wire yMin] [$wire xMax] [$wire yMax]]
    }
    if {[dict size $wire_groups] > 1} {
      dict for {position wires} $wire_groups {
        lappend channels [odb::shrinkSet [odb::bloatSet [odb::orSets $wires] 0 $lower_pitch_check] 0 $lower_pitch_check]
      }
    }

    set channels [odb::orSets $channels]
    # debug "Channel wires: [llength [odb::getRectangles $channel_wires]]"
    # debug "Channels: [llength [odb::getRectangles $channels]]"
  } else {
    set channel_wires [odb::subtractSet $stripe_locs($lower_layer_name,$net) [odb::bloatSet [odb::shrinkSet $stripe_locs($lower_layer_name,$net) 0 $upper_pitch_check] 0 $upper_pitch_check]]
    # Group wires with same yMin and yMax so that the channels form rectangles
    foreach wire [odb::getRectangles $channel_wires] {
      set yMin [$wire yMin]
      set yMax [$wire yMax]
      dict lappend wire_groups "$yMin,$yMax" [odb::newSetFromRect [$wire xMin] [$wire yMin] [$wire xMax] [$wire yMax]]
    }
    if {[dict size $wire_groups] > 1} {
      dict for {position wires} $wire_groups {
        lappend channels [odb::shrinkSet [odb::bloatSet [odb::orSets $wires] $lower_pitch_check 0] $lower_pitch_check 0]
      }
    }

    set channels [odb::orSets $channels]
    # debug "Channel wires: [llength [odb::getRectangles $channel_wires]]"
    # debug "Channels: [llength [odb::getRectangles $channels]]"
  }

  # foreach rect [odb::getRectangles $channels] {
  #   debug "([ord::dbu_to_microns [$rect xMin]] [ord::dbu_to_microns [$rect yMin]]) - ([ord::dbu_to_microns [$rect xMax]] [ord::dbu_to_microns [$rect yMax]])"
  # }
  # debug "Number of channels [llength [::odb::getPolygons $channels]]"

  return $channels
}

proc sort_insts_by_x_and_y {inst1 inst2} {
  lassign [$inst1 getOrigin] x1 y1
  lassign [$inst2 getOrigin] x2 y2

  if {$x1 == $x2} {
    return [expr {$y1 - $y2}]
  }
  return [expr {$x1 - $x2}]
}

proc sort_insts_by_y {inst1 inst2} {
  lassign [$inst1 getOrigin] x1 y1
  lassign [$inst2 getOrigin] x2 y2

  return [expr {$y1 - $y2}]
}

proc sort_by_min_x {rect1 rect2} {
  return [expr {[$rect1 xMin] - [$rect2 xMin]}]
}

proc sort_by_min_y {rect1 rect2} {
  return [expr {[$rect1 yMin] - [$rect2 yMin]}]
}

proc rightmost_rect {rects} {
  set ordered [lsort -command sort_by_min_x $rects]
  set str ""
  foreach rect $ordered {
    set str "$str [$rect xMin]"
  }
  # debug $str
  return [lindex $ordered end]
}

proc lowest_rect {rects} {
  set ordered [lsort -command sort_by_min_y $rects]
  set str ""
  foreach rect $ordered {
    set str "$str [$rect yMin]"
  }
  # debug $str
  return [lindex [lsort -command sort_by_min_y $rects] 0]
}


proc repair_channel {channel layer_name net min_size} {
  variable stripe_locs

  if {[get_dir $layer_name] == "hor"} {
    set channel_height [$channel dx]
  } else {
    set channel_height [$channel dy]
  }
  set width [get_grid_wire_width $layer_name]

  set xMin [$channel xMin]
  set xMax [$channel xMax]
  set yMin [$channel yMin]
  set yMax [$channel yMax]

  set other_straps {}
  foreach net_name [get_pg_nets] {
    if {$net == $net_name} {continue}
    lappend other_straps $stripe_locs($layer_name,$net_name)
  }
  set channel_spacing [get_grid_channel_spacing $layer_name [expr $xMax - $xMin]]
  set shapes [odb::andSet [odb::newSetFromRect $xMin $yMin $xMax $yMax] [odb::orSets $other_straps]]
  set shapes [odb::shrinkSet [odb::bloatSet $shapes $channel_spacing] $channel_spacing]
  # debug "Number of rects = [llength [odb::getRectangles $shapes]]"
  set other_strap [lindex [odb::getRectangles $shapes] 0]

  if {[llength [odb::getRectangles $shapes]] > 0} {
    if {[get_dir $layer_name] == "hor"} {
      set center [expr ($xMax + $xMin) / 2]
      set mid_channel [expr ($yMax + $yMin) / 2]
      if {$xMax - $xMin < $min_size} {
        set xMin [expr $center - $min_size / 2]
        set xMax [expr $center + $min_size / 2]
      }
      set channel_spacing [get_grid_channel_spacing $layer_name [expr $xMax - $xMin]]

      set other_strap_mid [expr ([$other_strap yMin] + [$other_strap yMax]) / 2]
      if {($mid_channel <= $other_strap_mid) && ([$other_strap yMin] - $channel_spacing - $width > $yMin)} {
        # debug "Stripe below $other_strap"
        set stripe [odb::newSetFromRect $xMin [expr [$other_strap yMin] - $channel_spacing - $width] $xMax [expr [$other_strap yMin] - $channel_spacing]]
      } elseif {($mid_channel > $other_strap_mid) && ([$other_strap yMax] + $channel_spacing + $width < $yMax)} {
        # debug "Stripe above $other_strap"
        set stripe [odb::newSetFromRect $xMin [expr [$other_strap yMax] + $channel_spacing] $xMax [expr [$other_strap yMax] + $channel_spacing + $width]]
      } else {
        set stripe {}
        utl::warn PDN 9169 "Cannot fit additional $net horizontal strap in channel ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin]) - ([ord::dbu_to_microns $xMax], [ord::dbu_to_microns $yMax])"
      }
    } else {
      set center [expr ($yMax + $yMin) / 2]
      set mid_channel [expr ($xMax + $xMin) / 2]
      if {$yMax - $yMin < $min_size} {
        set yMin [expr $center - $min_size / 2]
        set yMax [expr $center + $min_size / 2]
      }
      set channel_spacing [get_grid_channel_spacing $layer_name [expr $yMax - $yMin]]

      set other_strap_mid [expr ([$other_strap xMin] + [$other_strap xMax]) / 2]
      if {($mid_channel <= $other_strap_mid) && ([$other_strap xMin] - $channel_spacing - $width > $xMin)} {
        # debug "Stripe left of $other_strap on layer $layer_name, spacing: $channel_spacing, width $width, strap_edge: [$other_strap xMin]"
        set stripe [odb::newSetFromRect [expr [$other_strap xMin] - $channel_spacing - $width] $yMin [expr [$other_strap xMin] - $channel_spacing] $yMax]
      } elseif {($mid_channel > $other_strap_mid) && ([$other_strap xMax] + $channel_spacing + $width < $xMax)} {
        # debug "Stripe right of $other_strap"
        set stripe [odb::newSetFromRect [expr [$other_strap xMax] + $channel_spacing] $yMin [expr [$other_strap xMax] + $channel_spacing + $width] $yMax]
      } else {
        set stripe {}
        utl::warn PDN 9170 "Cannot fit additional $net vertical strap in channel ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin]) - ([ord::dbu_to_microns $xMax], [ord::dbu_to_microns $yMax])"
      }
    }
  } else {
    if {[get_dir $layer_name] == "hor"} {
      set channel_spacing [get_grid_channel_spacing $layer_name [expr $xMax - $xMin]]
      set routing_grid [round_to_routing_grid $layer_name [expr ($yMax + $yMin - $channel_spacing) / 2]]
      if {([expr $routing_grid - $width / 2] < $yMin) || ([expr $routing_grid + $width / 2] > $yMax)} {
        utl::warn PDN 9171 "Channel ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin] [ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax]) too narrow. Channel on layer $layer_name must be at least [ord::dbu_to_microns [expr round(2.0 * $width + $channel_spacing)]] wide."
      }

      set stripe [odb::newSetFromRect $xMin [expr $routing_grid - $width / 2] $xMax [expr $routing_grid + $width / 2]]
    } else {
      set channel_spacing [get_grid_channel_spacing $layer_name [expr $yMax - $yMin]]
      set routing_grid [round_to_routing_grid $layer_name [expr ($xMax + $xMin - $channel_spacing) / 2]]

      if {([expr $routing_grid - $width / 2] < $xMin) || ([expr $routing_grid + $width / 2] > $xMax)} {
        utl::warn PDN 9172 "Channel ([ord::dbu_to_microns $xMin] [ord::dbu_to_microns $yMin] [ord::dbu_to_microns $xMax] [ord::dbu_to_microns $yMax]) too narrow. Channel on layer $layer_name must be at least [ord::dbu_to_microns [expr round(2.0 * $width + $channel_spacing)]] wide."
      }

      set stripe [odb::newSetFromRect [expr $routing_grid - $width / 2] $yMin [expr $routing_grid + $width / 2] $yMax]
    }
  }

  if {$stripe != {}} {
    set r [lindex [odb::getRectangles $stripe] 0]
    # debug "Add stripe: [ord::dbu_to_microns [$r xMin]] [ord::dbu_to_microns [$r yMin]] [ord::dbu_to_microns [$r xMax]] [ord::dbu_to_microns [$r yMax]]"
    add_stripe $layer_name $net $stripe
  }
}

proc channel_has_pg_strap {channel layer_name net}  {
  variable stripe_locs
  # debug "start, channel: $channel, layer: $layer_name"
  # debug "       ([ord::dbu_to_microns [$channel xMin]] [ord::dbu_to_microns [$channel yMin]]) - ([ord::dbu_to_microns [$channel xMax]] [ord::dbu_to_microns [$channel yMax]])"

  set power_strap 0
  set ground_strap 0

  if {[array names stripe_locs "$layer_name,$net"] != ""} {
    set channel_set [odb::newSetFromRect [$channel xMin] [$channel yMin] [$channel xMax] [$channel yMax]]
    set check_set [odb::andSet $stripe_locs($layer_name,$net) $channel_set]

    foreach rect [odb::getRectangles $check_set] {
      if {[get_dir $layer_name] == "ver" && [$rect dx] < [get_grid_wire_width $layer_name]} {continue}
      if {[get_dir $layer_name] == "hor" && [$rect dy] < [get_grid_wire_width $layer_name]} {continue}
      # debug "Overlap found"
      # debug "       Direction: [get_dir $layer_name]"
      # debug "       Layer width: [get_grid_wire_width $layer_name]"
      # debug "       ([ord::dbu_to_microns [$rect xMin]] [ord::dbu_to_microns [$rect yMin]]) - ([ord::dbu_to_microns [$rect xMax]] [ord::dbu_to_microns [$rect yMax]])"
      return 1
    }
  }

  # debug "end: channel needs repair"
  return 0
}

proc process_channels {} {
  set layers [get_grid_channel_layers]
  set lower_layers [lrange $layers 0 end-1]
  set upper_layers [lrange $layers 1 end]
  foreach lower_layer_name $lower_layers upper_layer_name $upper_layers {
    foreach net [get_pg_nets] {
      set channels [identify_channels $lower_layer_name $upper_layer_name $net]
      if {$channels == {}} {continue}
      # debug "Tag: $net, Channels found: [llength [odb::getPolygons $channels]]"
      foreach channel [::odb::getRectangles $channels] {
        if {![channel_has_pg_strap $channel $upper_layer_name $net]} {
          set next_upper_layer_idx [expr [lsearch -exact $layers $upper_layer_name] + 1]
          if {$next_upper_layer_idx < [llength $layers]} {
            set next_upper_layer [lindex $layers $next_upper_layer_idx]
            set min_size [expr [get_grid_wire_pitch $next_upper_layer] + [get_grid_wire_width $next_upper_layer]]
          } else {
            set min_size 0
          }
          # debug "Repair channel: ([ord::dbu_to_microns [$channel xMin]] [ord::dbu_to_microns [$channel yMin]])-([ord::dbu_to_microns [$channel xMax]] [ord::dbu_to_microns [$channel yMax]]]), net: $net, layer: $upper_layer_name, min_size: $min_size"
          repair_channel $channel $upper_layer_name $net $min_size
        }
      }
      merge_stripes
    }
  }
}

proc get_stdcell_plus_area {} {
  variable stdcell_area
  variable stdcell_plus_area

  if {$stdcell_area == ""} {
    get_stdcell_area
  }
  # debug "stdcell_area      [get_extent $stdcell_area]"
  # debug "stdcell_plus_area [get_extent $stdcell_plus_area]"
  return $stdcell_plus_area
}

proc get_stdcell_area {} {
  variable stdcell_area
  variable stdcell_plus_area

  if {$stdcell_area != ""} {return $stdcell_area}
  set rails_width [get_rails_max_width]

  set rows [[ord::get_db_block] getRows]
  set first_row [[lindex $rows 0] getBBox]

  set minX [$first_row xMin]
  set maxX [$first_row xMax]
  set minY [$first_row yMin]
  set maxY [$first_row yMax]
  set stdcell_area [odb::newSetFromRect $minX $minY $maxX $maxY]
  set stdcell_plus_area [odb::newSetFromRect $minX [expr $minY - $rails_width / 2] $maxX [expr $maxY + $rails_width / 2]]

  foreach row [lrange $rows 1 end] {
    set box [$row getBBox]
    set minX [$box xMin]
    set maxX [$box xMax]
    set minY [$box yMin]
    set maxY [$box yMax]
    set stdcell_area [odb::orSet $stdcell_area [odb::newSetFromRect $minX $minY $maxX $maxY]]
    set stdcell_plus_area [odb::orSet $stdcell_plus_area [odb::newSetFromRect $minX [expr $minY - $rails_width / 2] $maxX [expr $maxY + $rails_width / 2]]]
  }

  return $stdcell_area
}

proc find_core_area {} {
  variable block

  set area [get_stdcell_area]

  return [get_extent $area]
}

proc get_rails_max_width {} {
  variable design_data
  variable default_grid_data

  set max_width 0
  foreach layer [get_rails_layers] {
     if {[dict exists $default_grid_data rails $layer]} {
       if {[set width [dict get $default_grid_data rails $layer width]] > $max_width} {
         set max_width $width
       }
     }
  }
  if {![dict exists $default_grid_data units]} {
    set max_width [ord::microns_to_dbu $max_width]
  }
  return $max_width
}


# This is a proc to get the first voltage domain that overlaps with the input box
proc get_voltage_domain {llx lly urx ury} {
  variable block
  variable design_data
  variable voltage_domains


  set name [dict get $design_data core_domain]
  foreach domain_name [dict keys $voltage_domains] {
    if {$domain_name == [dict get $design_data core_domain]} {continue}
    set domain [$block findRegion $domain_name]
    set rect [lindex [$domain getBoundaries] 0]

    set domain_xMin [$rect xMin]
    set domain_yMin [$rect yMin]
    set domain_xMax [$rect xMax]
    set domain_yMax [$rect yMax]

    if {!($domain_yMin >= $ury || $domain_xMin >= $urx || $domain_xMax <= $llx || $domain_yMax <= $lly)} {
      set name [$domain getName]
      break
    }
  }
  return $name
}

proc get_voltage_domain_switched_power {domain} {
  variable voltage_domains

  if {[dict exists $voltage_domains $domain switched_power]} {
    return [dict get $voltage_domains $domain switched_power]
  } else {
    return []
  }
}

proc get_voltage_domain_power {domain} {
  variable voltage_domains

  return [dict get $voltage_domains $domain primary_power]
}

proc get_voltage_domain_secondary_power {domain} {
  variable voltage_domains

  if {[dict exists $voltage_domains $domain secondary_power]} {
    return [dict get $voltage_domains $domain secondary_power]
  } else {
    return []
  }
}

proc get_voltage_domain_ground {domain} {
  variable voltage_domains

  return [dict get $voltage_domains $domain primary_ground]
}

# This proc is to split core domain's power stripes if they cross interal voltage domains that have different pwr/gnd nets
proc update_mesh_stripes_with_voltage_domains {net_name lay} {
  variable block
  variable stripes
  variable grid_data
  variable design_data
  variable voltage_domains

  set rails_width [get_rails_max_width]

  set stdcell_area [get_extent [get_stdcell_area]]
  set stdcell_min_x [lindex $stdcell_area 0]
  set stdcell_min_y [lindex $stdcell_area 1]
  set stdcell_max_x [lindex $stdcell_area 2]
  set stdcell_max_y [lindex $stdcell_area 3]

  set ring_adjustment 0
  if {[set ring_vertical_layer [get_core_ring_vertical_layer_name]] != ""} {
    if {[dict exists $grid_data core_ring $ring_vertical_layer pad_offset]} {
      set pad_area [find_pad_offset_area]
      set offset [expr [dict get $grid_data core_ring $ring_vertical_layer pad_offset]]
      set ring_adjustment [expr $stdcell_min_x - ([lindex $pad_area 0] + $offset)]
    }
    if {[dict exists $grid_data core_ring $ring_vertical_layer core_offset]} {
      set ring_adjustment [expr \
        [dict get $grid_data core_ring $ring_vertical_layer core_offset] + \
        [dict get $grid_data core_ring $ring_vertical_layer spacing] + \
        3 * [dict get $grid_data core_ring $ring_vertical_layer width] / 2 \
      ]
    }
  }

  set first_row [lindex [$block getRows] 0]
  set row_site [$first_row getSite]
  set site_width [$row_site getWidth]
  set row_height [$row_site getHeight]

  # This voltage domain to core domain margin is hard coded for now
  set MARGIN 6
  set X_MARGIN [expr ($MARGIN * $row_height / $site_width) * $site_width]
  set Y_MARGIN [expr $MARGIN * $row_height]

  foreach domain_name [dict keys $voltage_domains] {
    if {$domain_name == [dict get $design_data core_domain]} {continue}
    set domain [$block findRegion $domain_name]
    set first_rect [lindex [$domain getBoundaries] 0]

    # voltage domain area
    set domain_xMin [expr [$first_rect xMin]]
    set domain_yMin [expr [$first_rect yMin]]
    set domain_xMax [expr [$first_rect xMax]]
    set domain_yMax [expr [$first_rect yMax]]

    # voltage domain area + margin
    set domain_boundary_xMin [expr [$first_rect xMin] - $X_MARGIN]
    set domain_boundary_yMin [expr [$first_rect yMin] - $Y_MARGIN + $rails_width / 2]
    set domain_boundary_xMax [expr [$first_rect xMax] + $X_MARGIN]
    set domain_boundary_yMax [expr [$first_rect yMax] + $Y_MARGIN - $rails_width / 2]

    if {[get_dir $lay] == "hor"} {
      if {$domain_boundary_xMin < $stdcell_min_x + $site_width} {
        set domain_boundary_xMin [expr $stdcell_min_x - $ring_adjustment]
      }
      if {$domain_boundary_xMax > $stdcell_max_x - $site_width} {
        set domain_boundary_xMax [expr $stdcell_max_x + $ring_adjustment]
      }
    } else {
      if {$domain_boundary_yMin < $stdcell_min_y + $row_height} {
        set domain_boundary_yMin [expr $stdcell_min_y - $ring_adjustment]
      }
      if {$domain_boundary_yMax > $stdcell_max_y - $row_height} {
        set domain_boundary_yMax [expr $stdcell_max_y + $ring_adjustment]
      }
    }

    # Core domain's pwr/gnd nets that are not shared should not cross the entire voltage domain area
    set boundary_box \
        [odb::newSetFromRect \
          $domain_boundary_xMin \
          $domain_boundary_yMin \
          $domain_boundary_xMax \
          $domain_boundary_yMax \
        ]

    if {[get_dir $lay] == "hor"} {
      set domain_box \
        [odb::newSetFromRect \
          $domain_boundary_xMin \
          $domain_yMin \
          $domain_boundary_xMax \
          $domain_yMax \
        ]
    } else {
      set domain_box \
        [odb::newSetFromRect \
          $domain_xMin \
          $domain_boundary_yMin \
          $domain_xMax \
          $domain_boundary_yMax \
        ]
    }
    # Core domain's pwr/gnd nets shared with a voltage domain should not cross the domains' pwr/gnd rings
    set boundary_box_for_crossing_core_net [odb::subtractSet $boundary_box $domain_box]

    for {set i 0} {$i < [llength $stripes($lay,$net_name)]} {incr i} {
      set updated_polygonSet [lindex $stripes($lay,$net_name) $i]
      # Check if core domain's power is the same as voltage domain's power
      if {$net_name == [get_voltage_domain_power $domain_name] ||
          $net_name == [get_voltage_domain_ground $domain_name]} {
        set updated_polygonSet [odb::subtractSet $updated_polygonSet $boundary_box_for_crossing_core_net]
      } else {
        set updated_polygonSet [odb::subtractSet $updated_polygonSet $boundary_box]
      }
      # This if statemet prevents from deleting domain rings
      if {[llength [odb::getPolygons $updated_polygonSet]] > 0} {
        set stripes($lay,$net_name) [lreplace $stripes($lay,$net_name) $i $i $updated_polygonSet]
      }
    }
  }
}

# This proc is to check if a pwr/gnd net is unique for all voltage domains, the setWildConnected can be used
proc check_snet_is_unique {net} {
  variable voltage_domains

  set is_unique_power 1
  foreach vd_key [dict keys $voltage_domains] {
    if {[dict get $voltage_domains $vd_key primary_power] != [$net getName]} {
      set is_unique_power 0
      break
    }
  }

  set is_unique_ground 1
  foreach vd_key [dict keys $voltage_domains] {
    if {[dict get $voltage_domains $vd_key primary_ground] != [$net getName]} {
      set is_unique_ground 0
      break
    }
  }

  return [expr $is_unique_power || $is_unique_ground]

}

# This proc generates power rings for voltage domains, tags for the core domain are POWER/GROUND, tags for the other
# voltage domains are defined as POWER_<pwr-net> and GROUND_<gnd-net>
proc generate_voltage_domain_rings {core_ring_data} {
  variable block
  variable voltage_domains
  variable grid_data
  variable design_data

  foreach domain_name [dict keys $voltage_domains] {
    if {$domain_name == [dict get $design_data core_domain]} {continue}
    set pg_nets [get_pg_nets $voltage_domain]
    set power_net [get_voltage_domain_power $domain_name]
    set secondary_power_net [get_voltage_domain_secondary_power $domain_name]
    set ground_net [get_voltage_domain_ground $domain_name]

    set domain [$block findRegion $domain_name]
    set rect [lindex [$domain getBoundaries] 0]
    set area [list [$rect xMin] [$rect yMin] [$rect xMax] [$rect yMax]]

    if {[dict exists $layer_info pad_offset]} {
      # debug "pad_offset"
      set area [find_pad_offset_area]
      set offset [expr [dict get $layer_info pad_offset] + $width / 2]
      set pg_nets [lreverse [get_pg_nets]]

      set lx [expr [lindex $area 0] + $offset]
      set ly [expr [lindex $area 1] + $offset]
      set ux [expr [lindex $area 2] - $offset]
      set uy [expr [lindex $area 3] - $offset]
    } else {
      # debug "core_offset"
      set area [list {*}[[ord::get_db_core] ll] {*}[[ord::get_db_core] ur]]
      set offset [expr [dict get $layer_info core_offset] + $width / 2]
      set pg_nets [get_pg_nets]

      set lx [expr [lindex $area 0] - $offset]
      set ly [expr [lindex $area 1] - $offset]
      set ux [expr [lindex $area 2] + $offset]
      set uy [expr [lindex $area 3] + $offset]
    }

    dict for {layer layer_info} $core_ring_data {
      set spacing [dict get $layer_info spacing]
      set width [dict get $layer_info width]

      if {[dict exists $layer_info pad_offset]} {
        # debug "pad_offset"
        set area [find_pad_offset_area]
        set offset [expr [dict get $layer_info pad_offset] + $width / 2]
        set pg_nets [lreverse [get_pg_nets $voltage_domain]]

        set lx [expr [lindex $area 0] + $offset]
        set ly [expr [lindex $area 1] + $offset]
        set ux [expr [lindex $area 2] - $offset]
        set uy [expr [lindex $area 3] - $offset]
      } else {
        # debug "core_offset"
        set area [list {*}[[ord::get_db_core] ll] {*}[[ord::get_db_core] ur]]
        set offset [expr [dict get $layer_info core_offset] + $width / 2]
        set pg_nets [get_pg_nets $voltage_domain]

        set lx [expr [lindex $area 0] - $offset]
        set ly [expr [lindex $area 1] - $offset]
        set ux [expr [lindex $area 2] + $offset]
        set uy [expr [lindex $area 3] + $offset]
      }

    # debug "layer: $layer, area: $area, offset: $offset, pg_nets: $pg_nets, spacing: $spacing, width: $width"

      set core_ring_area [add_core_ring $net $layer $width $lx $ly $ux $uy]
      dict set grid_data core_ring_area $layer $core_ring_area

      if {[dict exists $layer_info pad_offset]} {
        set lx [expr $lx + $spacing + $width]
        set ly [expr $ly + $spacing + $width]
        set ux [expr $ux - $spacing - $width]
        set uy [expr $uy - $spacing - $width]
      } else {
        set lx [expr $lx - $spacing - $width]
        set ly [expr $ly - $spacing - $width]
        set ux [expr $ux + $spacing + $width]
        set uy [expr $uy + $spacing + $width]
      }
    }
  }
}


# This proc detects pins used in pdn.cfg for global connections
proc get_valid_mterms {net_name} {
  variable global_connections
  variable default_global_connections

  if {$global_connections == {}} {
    set global_connections $default_global_connections
  }

  set mterms_list {}
  if {![dict exists $global_connections $net_name]} {
    utl::error PDN  174 "Net $net_name has no global connections defined."
  }

  foreach pattern [dict get $global_connections $net_name] {
    lappend mterms_list [dict get $pattern pin_name]
  }
  return $mterms_list
}

proc core_area_boundary {} {
  variable design_data
  variable template
  variable metal_layers
  variable grid_data

  set core_area [find_core_area]
  # We need to allow the rails to extend by half a rails width in the y direction, since the rails overlap the core_area

  set llx [lindex $core_area 0]
  set lly [lindex $core_area 1]
  set urx [lindex $core_area 2]
  set ury [lindex $core_area 3]

  if {[dict exists $template width]} {
    set width [dict get $template width]
  } else {
    set width 2000
  }
  if {[dict exists $template height]} {
    set height [dict get $template height]
  } else {
    set height 2000
  }

  # Add blockages around the outside of the core area in order to trim back the templates.
  #
  set boundary [odb::newSetFromRect [expr $llx - $width] [expr $lly - $height] $llx [expr $ury + $height]]
  set boundary [odb::orSet $boundary [odb::newSetFromRect [expr $llx - $width] [expr $lly - $height] [expr $urx + $width] $lly]]
  set boundary [odb::orSet $boundary [odb::newSetFromRect [expr $llx - $width] $ury [expr $urx + $width] [expr $ury + $height]]]
  set boundary [odb::orSet $boundary [odb::newSetFromRect $urx [expr $lly - $height] [expr $urx + $width] [expr $ury + $height]]]
  set boundary [odb::subtractSet $boundary [get_stdcell_plus_area]]

  foreach layer $metal_layers {
    if {[dict exists $grid_data core_ring] && [dict exists $grid_data core_ring $layer]} {continue}
    dict set blockages $layer $boundary
  }

  return $blockages
}

proc get_instance_blockages {insts} {
  variable instances
  set blockages {}

  foreach inst $insts {
    foreach layer [get_macro_blockage_layers $inst] {
      # debug "Inst $inst"
      # debug "Macro boundary: [dict get $instances $inst macro_boundary]"
      # debug "Halo boundary:  [dict get $instances $inst halo_boundary]"
      set box [odb::newSetFromRect [get_instance_llx $inst] [get_instance_lly $inst] [get_instance_urx $inst] [get_instance_ury $inst]]
      if {[dict exists $blockages $layer]} {
        dict set blockages $layer [odb::orSet [dict get $blockages $layer] $box]
      } else {
        dict set blockages $layer $box
      }
    }
  }

  return $blockages
}

proc define_template_grid {} {
  variable design_data
  variable template
  variable plan_template
  variable block
  variable default_grid_data
  variable default_template_name

  set core_area [dict get $design_data config core_area]
  set llx [lindex $core_area 0]
  set lly [lindex $core_area 1]
  set urx [lindex $core_area 2]
  set ury [lindex $core_area 3]

  set core_width  [expr $urx - $llx]
  set core_height [expr $ury - $lly]

  set template_width  [dict get $template width]
  set template_height [dict get $template height]
  set x_sections [expr round($core_width  / $template_width)]
  set y_sections [expr round($core_height / $template_height)]

  dict set template offset x [expr [lindex $core_area 0] + ($core_width - $x_sections * $template_width) / 2]
  dict set template offset y [expr [lindex $core_area 1] + ($core_height - $y_sections * $template_height) / 2]

  if {$default_template_name == {}} {
    set template_name [lindex [dict get $default_grid_data template names] 0]
  } else {
    set template_name $default_template_name
  }

  for {set i -1} {$i <= $x_sections} {incr i} {
    for {set j -1} {$j <= $y_sections} {incr j} {
      set llx [expr $i * $template_width + [dict get $template offset x]]
      set lly [expr $j * $template_height + [dict get $template offset y]]

      dict set plan_template $llx $lly $template_name
    }
  }
  write_template_placement
}

proc set_blockages {these_blockages} {
  variable blockages

  set blockages $these_blockages
}

proc get_blockages {} {
  variable blockages

  return $blockages
}

proc add_blockage {layer blockage} {
  variable blockages

  if {[dict exists $blockages $layer]} {
    dict set blockages $layer [odb::orSet [dict get $blockages $layer] $blockage]
  } else {
    dict set blockages $layer $blockage
  }
}

proc add_padcell_blockage {layer blockage} {
  variable padcell_blockages

  if {[dict exists $padcell_blockages $layer]} {
    dict set padcell_blockages $layer [odb::orSet [dict get $padcell_blockages $layer] $blockage]
  } else {
    dict set padcell_blockages $layer $blockage
  }
}

proc apply_padcell_blockages {} {
  variable padcell_blockages
  variable global_connections

  dict for {layer_name blockages} $padcell_blockages {
    add_blockage $layer_name $blockages
  }
}

proc add_blockages {more_blockages} {
  variable blockages

  dict for {layer blockage} $more_blockages {
    add_blockage $layer $blockage
  }
}

proc add_macro_based_grids {} {
  variable grid_data
  variable design_data
  variable verbose

  if {![dict exists $design_data grid macro]} {return}

  foreach {key grid_data} [dict get $design_data grid macro] {
    if {![dict exists $grid_data _related_instances]} {continue}
    set instances [dict get $grid_data _related_instances]

    set_blockages {}
    if {[llength [dict keys $instances]] > 0} {
      utl::info PDN 9010 "Inserting macro grid for [llength [dict keys $instances]] macros."
      foreach instance [dict keys $instances] {
        if {$verbose == 1} {
          utl::info PDN 9034 "  - grid [dict get $grid_data name] for instance $instance"
        }

        if {[dict exists $grid_data grid_over_pg_pins]} {
          # debug "Grid over pins: [get_instance_pg_pins_area $instance]"
          dict set grid_data area [get_instance_pg_pins_area $instance]
        } else {
          # debug "Grid boundary: [get_instance_pg_pins_area $instance]"
          dict set grid_data area [dict get $instances $instance macro_boundary]
        }
        add_grid

        # debug "Generate vias for [get_all_pg_nets]"
        foreach net [get_all_pg_nets] {
          generate_grid_vias $net
        }
      }
    }
  }
}

proc plan_grid {} {
  variable design_data
  variable instances
  variable default_grid_data
  variable grid_data

  ################################## Main Code #################################

  if {![dict exists $design_data grid macro]} {
    utl::warn PDN 9018 "No macro grid specifications found - no straps added for macros."
  }

  utl::info PDN 9011 "****** INFO ******"

  print_strategy stdcell [get_stdcell_specification]

  if {[dict exists $design_data grid macro]} {
    dict for {name specification} [dict get $design_data grid macro] {
      print_strategy macro $specification
    }
  }

  utl::info PDN 9012 "**** END INFO ****"

  set specification $default_grid_data
  if {[dict exists $specification name]} {
    utl::info PDN 9013 "Inserting stdcell grid - [dict get $specification name]."
  } else {
    utl::info PDN 9014 "Inserting stdcell grid."
  }

  if {![dict exists $specification area]} {
    dict set specification area [dict get $design_data config core_area]
  }

  set grid_data $specification

  set boundary [odb::newSetFromRect {*}[dict get $grid_data area]]
  set insts_in_core_area [filtered_insts_within $instances $boundary]

  set_blockages [get_instance_blockages [dict keys $insts_in_core_area]]
  add_blockages [core_area_boundary]
  if {[dict exists $specification template]} {
    read_template_placement
  }

  add_grid
  process_channels

  foreach net [get_all_pg_nets] {
    generate_grid_vias $net
  }

  add_macro_based_grids
}

proc opendb_update_grid {} {
  utl::info PDN 9015 "Writing to database."
  export_opendb_vias
  export_opendb_specialnets
  export_opendb_power_pins
}

proc set_verbose {} {
  variable verbose

  set verbose 1
}

proc apply {args} {
  variable verbose
  variable default_grid_data
  variable global_connections

  # debug "global: $global_connections"
  if {[llength $args] > 0 && $verbose} {
    set config [lindex $args 0]
    utl::info PDN 9016 "Power Delivery Network Generator: Generating PDN\n  config: $config"
  }

  if {[llength $args] == 1} {
    set PDN_cfg [lindex $args 0]
    if {![file exists $PDN_cfg]} {
      utl::error PDN 9062 "File $PDN_cfg does not exist."
    }

    if {![file_exists_non_empty $PDN_cfg]} {
      utl::error PDN 9028 "File $PDN_cfg is empty."
    }
    source $PDN_cfg
  }

  init {*}$args
  complete_macro_grid_specifications
  set default_grid_data [get_stdcell_specification]

  plan_grid

  write_pdn_strategy

  opendb_update_grid
}
}
# tclint-enable
# tclfmt-enable
