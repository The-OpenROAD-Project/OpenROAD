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

sta::define_cmd_args "global_connect" {}

proc global_connect {args} {
  pdn::global_connect [ord::get_db_block]
}

sta::define_cmd_args "add_global_connection" {-net <net_name> \
                                              -inst_pattern <inst_name_pattern> \
                                              -pin_pattern <pin_name_pattern> \
                                              [(-power|-ground)] \
                                              [-defer_connection]
}

proc add_global_connection {args} {
  sta::parse_key_args "add_global_connection" args \
    keys {-net -inst_pattern -pin_pattern} \
    flags {-power -ground -defer_connection}

  sta::check_argc_eq0 "add_global_connection" $args

  pdn::check_design_state "add_global_connection"

  if {[info exists flags(-power)] && [info exists flags(-ground)]} {
    utl::error PDN 92 "The flags -power and -ground of the add_global_connection command are mutually exclusive."
  }

  if {![info exists keys(-net)]} {
    utl::error PDN 93 "The -net option of the add_global_connection command is required."
  }

  if {![info exists keys(-inst_pattern)]} {
    set keys(-inst_pattern) {.*}
  } else {
    if {[catch {regexp $keys(-inst_pattern) ""}]} {
      utl::error PDN 142 "The -inst_pattern argument ($keys(-inst_pattern)) is not a valid regular expression."
    }
  }

  if {![info exists keys(-pin_pattern)]} {
    utl::error PDN 94 "The -pin_pattern option of the add_global_connection command is required."
  } else {
    if {[catch {regexp $keys(-pin_pattern) ""}]} {
      utl::error PDN 157 "The -pin_pattern argument ($keys(-pin_pattern)) is not a valid regular expression."
    }
  }

  set net [[ord::get_db_block] findNet $keys(-net)]
  if {$net == "NULL"} {
    set net [odb::dbNet_create [ord::get_db_block] $keys(-net)]
    $net setSpecial
    if {![info exists flags(-power)] && ![info exists flags(-ground)]} {
      utl::warn PDN 167 "Net created for $keys(-net), if intended as power or ground net add the -power/-ground switch as appropriate."
    }
  }

  if {[info exists flags(-power)]} {
    $net setSpecial
    $net setSigType POWER
  } elseif {[info exists flags(-ground)]} {
    $net setSpecial
    $net setSigType GROUND
  }

  pdn::add_global_connect $keys(-inst_pattern) $keys(-pin_pattern) $net

  if {![info exists flags(-defer_connection)]} {
    global_connect
  }
}

sta::define_cmd_args "pdngen" {[-skip_trim] \
                               [-dont_add_pins] \
                               [-reset] \
                               [-ripup] \
                               [-report_only]
}

proc pdngen { args } {
  sta::parse_key_args "pdngen" args \
    keys {} flags {-skip_trim -dont_add_pins -reset -ripup -report_only -verbose}

  sta::check_argc_eq0or1  "pdngen" $args

  pdn::depricated flags -verbose

  if {[llength $args] == 1} {
    utl::warn PDN 1000 "Using legacy PDNGEN.\nConsider using \"convert_pdn_config $args\" to convert the legacy configuration."
    set config_file [file nativename [lindex $args 0]]
    pdngen::apply $config_file
  } else {
    sta::check_argc_eq0  "pdngen" $args

    if {[info exists flags(-reset)]} {
      if {[array size flags] != 1} {
        utl::error PDN 1037 "-reset flag is mutually exclusive to all other flags"
      }
      pdn::reset
      return
    }
    if {[info exists flags(-ripup)]} {
      if {[array size flags] != 1} {
        utl::error PDN 1038 "-ripup flag is mutually exclusive to all other flags"
      }
      pdn::rip_up
      return
    }
    if {[info exists flags(-report_only)]} {
      if {[array size flags] != 1} {
        utl::error PDN 1039 "-report_only flag is mutually exclusive to all other flags"
      }
      pdn::report
      return
    }

    set trim [expr [info exists flags(-skip_trim)] == 0]
    set add_pins [expr [info exists flags(-dont_add_pins)] == 0]

    pdn::build_grids $trim
    pdn::write_to_db $add_pins
  }
}

sta::define_cmd_args "set_voltage_domain" {[-name domain_name] \
                                           -power power_net_name \
                                           -ground ground_net_name \
                                           [-region region_name] \
                                           [-secondary_power secondary_power_net_name]}

proc set_voltage_domain {args} {
  sta::parse_key_args "set_voltage_domain" args \
    keys {-name -region -power -ground -secondary_power}

  sta::check_argc_eq0 "set_voltage_domain" $args

  pdn::check_design_state "set_voltage_domain"

  if {![info exists keys(-power)]} {
    utl::error PDN 1001 "The -power argument is required."
  } else {
    set pwr [[ord::get_db_block] findNet $keys(-power)]
    if {$pwr == "NULL"} {
      utl::error PDN 1002 "Unable to find power net: $keys(-power)"
    }
  }

  if {![info exists keys(-ground)]} {
    utl::error PDN 1003 "The -ground argument is required."
  } else {
    set gnd [[ord::get_db_block] findNet $keys(-ground)]
    if {$gnd == "NULL"} {
      utl::error PDN 1004 "Unable to find ground net: $keys(-ground)"
    }
  }

  set region "NULL"
  set name "NULL"
  if {[info exists keys(-region)]} {
    set region [[ord::get_db_block] findRegion $keys(-region)]
    if {$region == "NULL"} {
      utl::error PDN 1005 "Unable to find region: $keys(-region)"
    }

    if {[info exists keys(-name)]} {
      set name $key(-name)
      if {$name == "CORE"} {
        set name "Core"
      }
    } else {
      set name [$region getName]
    }
  }

  set secondary {}
  if {[info exists keys(-secondary_power)]} {
    foreach snet $keys(-secondary_power) {
      set db_net [[ord::get_db_block] findNet $snet]
      if {$db_net == "NULL"} {
        utl::error PDN 1006 "Unable to find secondary power net: $snet"
      } else {
        lappend secondary $db_net
      }
    }
  }

  if {$region == "NULL"} {
    pdn::set_core_domain $pwr $gnd $secondary
  } else {
    pdn::make_region_domain $name $pwr $gnd $secondary $region
  }
}

sta::define_cmd_args "define_pdn_grid" {[-name <name>] \
                                        [-macro] \
                                        [-grid_over_pg_pins|-grid_over_boundary] \
                                        [-voltage_domains <list_of_voltage_domains>] \
                                        [-orient <list_of_valid_orientations>] \
                                        [-instances <list_of_instances>] \
                                        [-cells <list_of_cell_names> ] \
                                        [-halo <list_of_halo_values>] \
                                        [-pin_direction (horizontal|vertical)] \
                                        [-pins <list_of_pin_layers>] \
                                        [-starts_with (POWER|GROUND)]}

proc define_pdn_grid {args} {
  set is_macro 0
  foreach arg $args {
    if {$arg == "-macro"} {
      set is_macro 1
    }
  }
  if {$is_macro} {
    pdn::define_pdn_grid_macro {*}$args
  } else {
    pdn::define_pdn_grid {*}$args
  }
}

sta::define_cmd_args "add_pdn_stripe" {[-grid grid_name] \
                                       -layer layer_name \
                                       [-width width_value] \
                                       [-followpins] \
                                       [-extend_to_core_ring] \
                                       [-pitch pitch_value] \
                                       [-spacing spacing_value] \
                                       [-offset offset_value] \
                                       [-starts_width (POWER|GROUND)]
                                       [-extend_to_boundary] \
                                       [-snap_to_grid] \
                                       [-number_of_straps count]
}

proc add_pdn_stripe {args} {
  sta::parse_key_args "add_pdn_stripe" args \
    keys {-grid -layer -width -pitch -spacing -offset -starts_with -number_of_straps} \
    flags {-followpins -extend_to_core_ring -extend_to_boundary -snap_to_grid}

  sta::check_argc_eq0 "add_pdn_stripe" $args

  pdn::check_design_state "add_pdn_stripe"

  if {![info exists keys(-layer)]} {
    utl::error PDN 1007 "The -layer argument is required."
  }

  if {![info exists flags(-followpins)]} {
    if {![info exists keys(-width)]} {
      utl::error PDN 1008 "The -width argument is required."
    } else {
      set width $keys(-width)
    }

    if {![info exists keys(-pitch)]} {
      utl::error PDN 1009 "The -pitch argument is required."
    }
  }

  set width 0
  if {[info exists keys(-width)]} {
    set width $keys(-width)
  }

  set pitch 0
  if {[info exists keys(-pitch)]} {
    set pitch $keys(-pitch)
  }

  set spacing 0
  if {[info exists keys(-spacing)]} {
    set spacing $keys(-spacing)
  }

  set offset 0
  if {[info exists keys(-offset)]} {
    set offset $keys(-offset)
  }

  set number_of_straps 0
  if {[info exists keys(-number_of_straps)]} {
    set number_of_straps $keys(-number_of_straps)
  }

  set grid ""
  if {[info exists keys(-grid)]} {
    set grid $keys(-grid)
  }

  set layer [pdn::get_layer $keys(-layer)]
  set width [ord::microns_to_dbu $width]
  set pitch [ord::microns_to_dbu $pitch]
  set spacing [ord::microns_to_dbu $spacing]
  set offset [ord::microns_to_dbu $offset]

  set extend "Core"
  if {[info exists flags(-extend_to_core_ring)] && [info exists flags(-extend_to_boundary)]} {
    utl::error PDN 1010 "Options -extend_to_core_ring and -extend_to_boundary are mutually exclusive."
  } elseif {[info exists flags(-extend_to_core_ring)]} {
    set extend "Rings"
  } elseif {[info exists flags(-extend_to_boundary)]} {
    set extend "Boundary"
  }

  set use_grid_power_order 1
  set start_with_power 0
  if {[info exists keys(-starts_with)]} {
    set use_grid_power_order 0
    set start_with_power [pdn::get_starts_with $keys(-starts_with)]
  }

  if {[info exists flags(-followpins)]} {
    pdn::make_followpin $grid $layer $width $extend
  } else {
    pdn::make_strap $grid \
                    $layer \
                    $width \
                    $spacing \
                    $pitch \
                    $offset \
                    $number_of_straps \
                    [info exists flags(-snap_to_grid)] \
                    $use_grid_power_order \
                    $start_with_power \
                    $extend
  }
}

sta::define_cmd_args "add_pdn_ring" {[-grid grid_name] \
                                     -layers list_of_2_layer_names \
                                     -widths (width_value|list_of_width_values) \
                                     -spacings (spacing_value|list_of_spacing_values) \
                                     [-core_offsets (offset_value|list_of_offset_values)] \
                                     [-pad_offsets (offset_value|list_of_offset_values)] \
                                     [-add_connect] \
                                     [-extend_to_boundary] \
                                     [-connect_to_pads] \
                                     [-connect_to_pad_layers layers]}

proc add_pdn_ring {args} {
  sta::parse_key_args "add_pdn_ring" args \
    keys {-grid -layers -widths -spacings -core_offsets -pad_offsets -connect_to_pad_layers -power_pads -ground_pads} \
    flags {-add_connect -extend_to_boundary -connect_to_pads}

  sta::check_argc_eq0 "add_pdn_ring" $args

  pdn::check_design_state "add_pdn_ring"

  if {[pdn::depricated keys -power_pads ", use -connect_to_pads instead."] != {}} {
    set flags(-connect_to_pads) 1
  }
  if {[pdn::depricated keys -ground_pads ", use -connect_to_pads instead."] != {}} {
    set flags(-connect_to_pads) 1
  }

  if {![info exists keys(-layers)]} {
    utl::error PDN 1011 "The -layers argument is required."
  }

  if {[llength $keys(-layers)] != 2} {
    utl::error PDN 1012 "Expecting a list of 2 elements for -layers option of add_pdn_ring command, found [llength $keys(-layers)]."
  }

  if {![info exists keys(-widths)]} {
    utl::error PDN 1013 "The -widths argument is required."
  } else {
    set widths [pdn::get_one_to_two "-widths" $keys(-widths)]
  }

  if {![info exists keys(-spacings)]} {
    utl::error PDN 1014 "The -spacings argument is required."
  } else {
    set spacings [pdn::get_one_to_two "-spacings" $keys(-spacings)]
  }

  if {[info exists keys(-core_offsets)] && [info exists keys(-pad_offsets)]} {
    utl::error PDN 1015 "Only one of -pad_offsets or -core_offsets can be specified."
  } elseif {![info exists keys(-core_offsets)] && ![info exists keys(-pad_offsets)]} {
    utl::error PDN 1016 "One of -pad_offsets or -core_offsets must be specified."
  }
  set core_offsets "0 0 0 0"
  if {[info exists keys(-core_offsets)]} {
    set core_offsets [pdn::get_one_to_four "-core_offsets" $keys(-core_offsets)]
  }
  set pad_offsets "0 0 0 0"
  if {[info exists keys(-pad_offsets)]} {
    set pad_offsets [pdn::get_one_to_four "-pad_offsets" $keys(-pad_offsets)]
  }

  if {[info exists flags(-extend_to_boundary)] && [info exists flags(-connect_to_pads)]} {
    utl::error PDN 1017 "Only one of -pad_offsets or -core_offsets can be specified."
  }

  set grid ""
  if {[info exists keys(-grid)]} {
    set grid $keys(-grid)
  }

  set l0 [pdn::get_layer [lindex $keys(-layers) 0]]
  set l1 [pdn::get_layer [lindex $keys(-layers) 1]]
  set widths [list \
    [ord::microns_to_dbu [lindex $widths 0]] \
    [ord::microns_to_dbu [lindex $widths 1]]
  ]
  set spacings [list \
    [ord::microns_to_dbu [lindex $spacings 0]] \
    [ord::microns_to_dbu [lindex $spacings 1]]
  ]
  set core_offsets [list \
    [ord::microns_to_dbu [lindex $core_offsets 0]] \
    [ord::microns_to_dbu [lindex $core_offsets 1]] \
    [ord::microns_to_dbu [lindex $core_offsets 2]] \
    [ord::microns_to_dbu [lindex $core_offsets 3]]
  ]
  set pad_offsets [list \
    [ord::microns_to_dbu [lindex $pad_offsets 0]] \
    [ord::microns_to_dbu [lindex $pad_offsets 1]] \
    [ord::microns_to_dbu [lindex $pad_offsets 2]] \
    [ord::microns_to_dbu [lindex $pad_offsets 3]]
  ]
  set connect_to_pad_layers {}
  if {[info exists flags(-connect_to_pads)]} {
    if {![info exists keys(-connect_to_pad_layers)]} {
      foreach layer [[ord::get_db_tech] getLayers] {
        if {[$layer getType] == "ROUTING"} {
          lappend connect_to_pad_layers $layer
        }
      }
    } else {
      foreach layer $keys(-connect_to_pad_layers) {
        lappend connect_to_pad_layers [pdn::get_layer $layer]
      }
    }
  }

  pdn::make_ring $grid \
                 $l0 \
                 [lindex $widths 0] \
                 [lindex $spacings 0] \
                 $l1 \
                 [lindex $widths 1] \
                 [lindex $spacings 1] \
                 {*}$core_offsets \
                 {*}$pad_offsets \
                 [info exists flags(-extend_to_boundary)] \
                 $connect_to_pad_layers

  if {[info exists flags(-add_connect)]} {
    add_pdn_connect -grid $keys(-grid) -layers $keys(-layers)
  }
}

sta::define_cmd_args "add_pdn_connect" {[-grid grid_name] \
                                        -layers list_of_2_layers \
                                        [-cut_pitch pitch_value] \
                                        [-fixed_vias list_of_vias]}

proc add_pdn_connect {args} {
  sta::parse_key_args "add_pdn_connect" args \
    keys {-grid -layers -cut_pitch -fixed_vias} \
    flags {}

  sta::check_argc_eq0 "add_pdn_connect" $args

  pdn::check_design_state "add_pdn_connect"

  if {![info exists keys(-layers)]} {
    utl::error PDN 1019 "The -layers argument is required."
  } elseif {[llength $keys(-layers)] != 2} {
    utl::error PDN 1020 "The -layers must contain two layers."
  }

  set l0 [pdn::get_layer [lindex $keys(-layers) 0]]
  set l1 [pdn::get_layer [lindex $keys(-layers) 1]]

  set cut_pitch "0 0"
  if {[info exists keys(-cut_pitch)]} {
    set cut_pitch [pdn::get_one_to_two "-cut_pitch" $keys(-cut_pitch)]
  }
  set cut_pitch [list \
    [ord::microns_to_dbu [lindex $cut_pitch 0]]\
    [ord::microns_to_dbu [lindex $cut_pitch 1]]
  ]

  set grid ""
  if {[info exists keys(-grid)]} {
    set grid $keys(-grid)
  }

  set fixed_generate_vias {}
  set fixed_tech_vias {}
  if {[info exists keys(-fixed_vias)]} {
    foreach via $keys(-fixed_vias) {
      set tech_via [[ord::get_db_tech] findVia $via]
      set generate_via [[ord::get_db_tech] findViaGenerateRule $via]
      if {$tech_via == "NULL" && $generate_via == "NULL"} {
        utl::error PDN 1021 "Unable to find via: $via"
      }
      if {$tech_via != "NULL"} {
        lappend fixed_tech_vias $tech_via
      }
      if {$generate_via != "NULL"} {
        lappend fixed_generate_vias $generate_via
      }
    }
  }

  pdn::make_connect $grid $l0 $l1 {*}$cut_pitch $fixed_generate_vias $fixed_tech_vias
}

# conversion utility
sta::define_hidden_cmd_args  "convert_pdn_config" { config_file }
proc convert_pdn_config { args } {
  sta::parse_key_args "convert_pdn_config" args \
    keys {} \
    flags {}

  sta::check_argc_eq1 "convert_pdn_config" $args

  pdn::check_design_state "convert_pdn_config"

  pdn::convert_config $args
}

namespace eval pdn {

  proc check_design_state { args } {
    if {[ord::get_db_block] == "NULL"} {
      utl::error PDN 1022 "Design must be loaded before calling $args."
    }
  }

  proc get_layer { name } {
    set layer [[ord::get_db_tech] findLayer $name]
    if {$layer == "NULL"} {
      utl::error PDN 1023 "Unable to find $name layer."
    } else {
      return $layer
    }
  }

  proc depricated { args_var key {use "."}} {
    upvar 1 $args_var args
    if {[info exists args($key)]} {
      utl::warn PDN 1024 "$key has been deprecated$use"
      return $args($key)
    }
    return {}
  }

  proc define_pdn_grid { args } {
    sta::parse_key_args "define_pdn_grid" args \
      keys {-name -voltage_domains -pins -starts_with} \
      flags {}

    sta::check_argc_eq0 "define_pdn_grid" $args
    pdn::check_design_state "define_pdn_grid"

    if {[info exists keys(-voltage_domains)]} {
      set domains [pdn::get_voltage_domains $keys(-voltage_domains)]
    } else {
      set domains [pdn::find_domain "Core"]
    }

    if {![info exists keys(-name)]} {
      utl::error PDN 1025 "-name is required"
    }

    set start_with_power 1
    if {[info exists keys(-starts_with)]} {
      set start_with_power [pdn::get_starts_with $keys(-starts_with)]
    }

    set pin_layers {}
    if {[info exists keys(-pins)]} {
      foreach pin $keys(-pins) {
        lappend pin_layers [pdn::get_layer $pin]
      }
    }

    foreach domain $domains {
      pdn::make_core_grid $domain $keys(-name) $start_with_power $pin_layers
    }
  }

  proc define_pdn_grid_macro { args } {
    sta::parse_key_args "define_pdn_grid" args \
    keys {-name -voltage_domains -orient -instances -cells -halo -pin_direction -starts_with} \
    flags {-macro -grid_over_pg_pins -grid_over_boundary}

    sta::check_argc_eq0 "define_pdn_grid" $args
    pdn::check_design_state "define_pdn_grid"

    pdn::depricated flags -grid_over_pg_pins
    pdn::depricated flags -grid_over_bounary
    pdn::depricated keys -pin_direction
    if {[info exists flags(-grid_over_pg_pins)] && [info exists flags(-grid_over_bounary)]} {
      utl::error PDN 1026 "Options -grid_over_pg_pins and -grid_over_boundary are mutually exclusive."
    }

    if {[info exists keys(-instances)] && [info exists keys(-cells)]} {
      utl::error PDN 1027 "Options -instances and -cells are mutually exclusive."
    } elseif {![info exists keys(-instances)] && ![info exists keys(-cells)]} {
      utl::error PDN 1028 "Either -instances or -cells must be specified."
    }

    if {![info exists keys(-name)]} {
       utl::error PDN 1029 "-name is required"
    }

    set start_with_power 1
    if {[info exists keys(-starts_with)]} {
      set start_with_power [pdn::get_starts_with $keys(-starts_with)]
    }

    set halo "0 0 0 0"
    if {[info exists keys(-halo)]} {
      set halo [pdn::get_one_to_four "-halo" $keys(-halo)]
    }
    set halo [list \
     [ord::microns_to_dbu [lindex $halo 0]] \
     [ord::microns_to_dbu [lindex $halo 1]] \
     [ord::microns_to_dbu [lindex $halo 2]] \
     [ord::microns_to_dbu [lindex $halo 3]]]

    if {[info exists keys(-voltage_domains)]} {
      set domains [pdn::get_voltage_domains $keys(-voltage_domains)]
    } else {
      set domains [pdn::find_domain "Core"]
    }

    if {[info exists keys(-instances)]} {
      foreach inst_name $keys(-instances) {
        set inst [[ord::get_db_block] findInst $inst_name]

        if {$inst == "NULL"} {
          utl::error PDN 1030 "Unable to find instance: $inst_name"
        }

        foreach domain $domains {
          pdn::make_instance_grid $domain $keys(-name) $start_with_power $inst {*}$halo
        }
      }
    } else {
      set orients {}
      if {[info exists keys(-orient)]} {
        set orients [pdn::get_orientations $keys(-orient)]
      }
      foreach cell $keys(-cells) {
        set cell_found 0
        foreach inst [[ord::get_db_block] getInsts] {
          if {[[$inst getMaster] getName] == $cell} {
            set add_cell 0
            if {[llength $orients] != 0} {
              if {[lsearch -exact $orients [$inst getOrient]] != -1} {
                set add_cell 1
              }
            } else {
              set add_cell 1
            }
            if {$add_cell} {
              set cell_found 1
              foreach domain $domains {
                pdn::make_instance_grid $domain $keys(-name) $start_with_power $inst {*}$halo
              }
            }
          }
        }

        if {!$cell_found} {
          utl::error PDN 1031 "Unable to find cell: $cell"
        }
      }
    }
  }

  proc get_voltage_domains { names } {
    set domains {}
    foreach name $names {
      set domain [pdn::find_domain $name]

      if {$domain == "NULL"} {
        utl::error PDN 1032 "Unable to find $name domain."
      }
      lappend domains $domain
    }
    return $domains
  }

  proc get_one_to_two { arg value } {
    if {[llength $value] == 1} {
      return [list $value $value]
    } elseif {[llength $value] == 2} {
      return $value
    } else {
      utl::error PDN 1033 "Argument $arg must consist of 1 or 2 entries."
    }
    return $list_val
  }

  proc get_one_to_four { arg value } {
    if {[llength $value] == 1} {
      return [list $value $value $value $value]
    } elseif {[llength $value] == 2} {
      return [list {*}$value {*}$value]
    } elseif {[llength $value] == 4} {
      return $value
    } else {
      utl::error PDN 1034 "Argument $arg must consist of 1, 2 or 4 entries."
    }
    return $list_val
  }

  proc get_starts_with { value } {
    if {$value == "POWER"} {
      return 1
    } elseif {$value == "GROUND"} {
      return 0
    } else {
      utl::error PDN 1035 "Unknown -starts_with option: $value"
    }
  }

  proc get_orientations {orientations} {
    set valid_orientations {R0 R90 R180 R270 MX MY MXR90 MYR90}
    set lef_orientations {N R0 FN MY S R180 FS MX E R270 FE MYR90 W R90 FW MXR90}

    set checked_orientations {}
    foreach orient $orientations {
      if {[lsearch -exact $valid_orientations $orient] > -1} {
        lappend checked_orientations $orient
      } elseif {[dict exists $lef_orientations $orient]} {
        lappend checked_orientations [dict get $lef_orientations $orient]
      } else {
        utl::error PDN 1036 "Invalid orientation $orient specified, must be one of [join $valid_orientations {, }]."
      }
    }
    return $checked_orientations
  }

  proc convert_header { title } {
    puts "####################################"
    puts "# $title"
    puts "####################################"
  }

  # helper function to convert config file to tcl commands
  proc convert_config { config } {
    pdngen::source_config $config

    convert_header "global connections"
    # build global connections
    set default_global_conns [generate_global_connect_commands $pdngen::default_global_connections]
    set global_conns [generate_global_connect_commands $pdngen::global_connections]
    if {$default_global_conns || $global_conns} {
      puts "global_connect"
    }

    convert_header "voltage domains"
    # build voltage domains
    dict for {name spec} $pdngen::voltage_domains {
      set command "set_voltage_domain -name \{$name\}"
      append command " -power \{[dict get $spec primary_power]\}"
      append command " -ground \{[dict get $spec primary_ground]\}"
      if {[dict exists $spec secondary_power]} {
        append command " -secondary_power \{[dict get $spec secondary_power]\}"
      }
      puts "$command"

      dict for {grid_name grid_spec} $spec {
        generate_grid_commands $name $grid_spec
      }
    }

    convert_header "standard cell grid"
    dict for {name spec} [dict get $pdngen::design_data grid stdcell] {
      set command [generate_grid_command $spec]

      puts "$command"

      generate_grid_commands $name $spec
    }

    convert_header "macro grids"
    if {[dict exists $pdngen::design_data grid macro]} {
      dict for {name spec} [dict get $pdngen::design_data grid macro] {
        convert_header "grid for: $name"
        set command [generate_grid_command $spec]
        append command " -macro"
        if {[dict exists $spec orient]} {
          append command " -orient \{[dict get $spec orient]\}"
        }
        if {[dict exists $spec halo]} {
          set halo {}
          foreach h [dict get $spec halo] {
            lappend halo [ord::dbu_to_micron $h]
          }
          append command " -halo \{$halo\}"
        }
        if {[dict exists $spec instances]} {
          append command " -instances \{[dict get $spec instances]\}"
        }
        if {[dict exists $spec macro]} {
          append command " -cells \{[dict get $spec macro]\}"
        }

        puts "$command"

        generate_grid_commands $name $spec
      }
    }
  }

  proc generate_global_connect_commands { conns } {
    set found 0
    dict for {net net_conn} $conns {
      set found 1
      set extra_append ""
      if {[lsearch -exact [pdngen::get_power_nets] $net] != -1} {
        set extra_append " -power"
      } elseif {[lsearch -exact [pdngen::get_ground_nets] $net] != -1} {
        set extra_append " -ground"
      }
      foreach conn $net_conn {
        set command "add_global_connection -defer_connection -net \{$net\}"
        if {[dict exists $conn inst_name]} {
          append command " -inst_pattern \{[dict get $conn inst_name]\}"
        }
        if {[dict exists $conn pin_name]} {
          append command " -pin_pattern \{[dict get $conn pin_name]\}"
        }
        append command $extra_append
        set extra_append ""
        puts $command
      }
    }
    return $found
  }

  proc generate_grid_command { spec } {
    set command "define_pdn_grid"
    append command " -name \{[dict get $spec name]\}"
    append command " -voltage_domains \{[dict get $spec voltage_domains]\}"
    if {[dict exists $spec starts_with]} {
      append command " -starts_with \{[dict get $spec starts_with]\}"
    }
    return $command
  }

  proc generate_grid_commands { grid_name spec } {
    if {[dict exists $spec rails]} {
      dict for {layer rail} [dict get $spec rails] {
        set command [generate_strap_commands $grid_name $layer $rail]
        append command " -followpins"
        puts "$command"
      }
    }
    if {[dict exists $spec core_ring]} {
      set command "add_pdn_ring -grid \{$grid_name\}"
      dict for {layer ring} [dict get $spec core_ring] {
        append command " -layer \{$layer\}"
        append command " -width \{[dict get $ring width]\}"
        append command " -spacing \{[dict get $ring spacing]\}"
        if {[dict exists $rail core_offset]} {
          append command " -offset \{[dict get $ring core_offset]\}"
        }
        if {[dict exists $rail pad_offset]} {
          append command " -pad_offset \{[dict get $ring pad_offset]\}"
        }
        if {[dict exists $spec pwr_pads] || [dict exists $spec gnd_pads]} {
          append command " -connect_to_pads"
        }
        puts "$command"
      }
    }
    if {[dict exists $spec straps]} {
      dict for {layer strap} [dict get $spec straps] {
        set command [generate_strap_commands $grid_name $layer $strap]
        puts "$command"
      }
    }
    if {[dict exists $spec connect]} {
      foreach connect [dict get $spec connect] {
        set command "add_pdn_connect -grid \{$grid_name\}"
        set l0 [regsub "(.*)(_PIN_(hor|ver))" [lindex $connect 0] "\\1"]
        set l1 [regsub "(.*)(_PIN_(hor|ver))" [lindex $connect 1] "\\1"]
        append command " -layer \{$l0 $l1\}"

        for {set c 2} {$c < [llength $connect]} {incr c} {
          if {[lindex $connect $c] == "fixed_vias"} {
            incr c
            append command " -fixed_vias \{[lindex $connect $c]\}"
          }
          if {[lindex $connect $c] == "constraints"} {
            incr c
          }
          if {[lindex $connect $c] == "cut_pitch"} {
            incr c
            append command " -cut_pitch \{[lindex $connect $c]\}"
          }
        }

        puts "$command"
      }
    }
  }

  proc generate_strap_commands { grid_name layer strap } {
    set command "add_pdn_stripe -grid \{$grid_name\}"
    append command " -layer \{$layer\}"
    append command " -width \{[dict get $strap width]\}"
    append command " -pitch \{[dict get $strap pitch]\}"
    append command " -offset \{[dict get $strap offset]\}"
    if {[dict exists $strap extend_to_core_ring]} {
      append command " -extend_to_core_rings"
    }
    if {[dict exists $strap starts_with]} {
      append command " -starts_with \{[dict get $strap starts_with]\}"
    }
    return $command
  }

  # namespace pdn
}

namespace eval pdngen {

  proc source_config { config } {
    if {![file exists $config]} {
      utl::error PDN 1040 "File $config does not exist."
    }
    if {[file size $config] == 0} {
      utl::error PDN 1041 "File $config is empty."
    }
    source $config
  }

  proc get_power_nets {} {
    if {[info vars ::power_nets] == ""} {
      return "VDD"
    } else {
      return $::power_nets
    }
  }

  proc get_ground_nets {} {
    if {[info vars ::ground_nets] == ""} {
      return "VSS"
    } else {
      return $::ground_nets
    }
  }

  # namespace pdngen
}
