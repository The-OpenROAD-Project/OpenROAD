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

sta::define_cmd_args "pdngen" {[-skip_trim] \
                               [-dont_add_pins] \
                               [-reset] \
                               [-ripup] \
                               [-report_only] \
                               [-failed_via_report file] \
                               [-verbose]
}

proc pdngen { args } {
  sta::parse_key_args "pdngen" args \
    keys {-failed_via_report} flags {-skip_trim -dont_add_pins -reset -ripup -report_only -verbose}

  sta::check_argc_eq0 "pdngen" $args

  if { [ord::get_db_block] == "NULL" } {
    utl::error PDN 2 "No design block found."
  }

  pdn::depricated flags -verbose

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

  set failed_via_report ""
  if {[info exists keys(-failed_via_report)]} {
    set failed_via_report $keys(-failed_via_report)
  }

  pdn::check_setup
  pdn::build_grids $trim
  pdn::write_to_db $add_pins $failed_via_report
  pdn::reset_shapes
}

sta::define_cmd_args "set_voltage_domain" {-name domain_name \
                                           -power power_net_name \
                                           -ground ground_net_name \
                                           [-region region_name] \
                                           [-secondary_power secondary_power_net_name] \
                                           [-switched_power switched_power_net_name]}

proc set_voltage_domain {args} {
  sta::parse_key_args "set_voltage_domain" args \
    keys {-name -region -power -ground -secondary_power -switched_power} flags {}

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
      set name [pdn::modify_voltage_domain_name $key(-name)]
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

  set switched_power "NULL"
  if {[info exists keys(-switched_power)]} {
    set switched_power_net_name $keys(-switched_power)
    set switched_power [[ord::get_db_block] findNet $switched_power_net_name]
    if {$switched_power == "NULL"} {
      set switched_power [odb::dbNet_create [ord::get_db_block] $switched_power_net_name]
      $switched_power setSpecial
      $switched_power setSigType POWER
    } else {
      set signal_type [$switched_power getSigType]
      if {$signal_type != "POWER"} {
        utl::error PDN 199 "Net $switched_power_net_name already exists in the design,\
          but is of signal type ${signal_type}."
      }
    }
  }

  if {$region == "NULL"} {
    if {[info exists keys(-name)] && [pdn::modify_voltage_domain_name $keys(-name)] != "Core"} {
      utl::warn PDN 1042 "Core voltage domain will be named \"Core\"."
    }
    pdn::set_core_domain $pwr $switched_power $gnd $secondary
  } else {
    pdn::make_region_domain $name $pwr $switched_power $gnd $secondary $region
  }
}

sta::define_cmd_args "define_pdn_grid" {[-name <name>] \
                                        [-macro] \
                                        [-existing] \
                                        [-grid_over_pg_pins|-grid_over_boundary] \
                                        [-voltage_domains <list_of_voltage_domains>] \
                                        [-orient <list_of_valid_orientations>] \
                                        [-instances <list_of_instances>] \
                                        [-cells <list_of_cell_names> ] \
                                        [-default] \
                                        [-halo <list_of_halo_values>] \
                                        [-pins <list_of_pin_layers>] \
                                        [-starts_with (POWER|GROUND)] \
                                        [-obstructions <list_of_layers>] \
                                        [-power_switch_cell <name>] \
                                        [-power_control <signal_name>] \
                                        [-power_control_network (STAR|DAISY)]
};#checker off

proc define_pdn_grid {args} {
  set is_macro 0
  set is_existing 0
  foreach arg $args {
    if {$arg == "-macro"} {
      set is_macro 1
    } elseif {$arg == "-existing"} {
      set is_existing 1
    }
  }
  if {$is_macro} {
    pdn::define_pdn_grid_macro {*}$args
  } elseif {$is_existing} {
    pdn::define_pdn_grid_existing {*}$args
  } else {
    pdn::define_pdn_grid {*}$args
  }
}

sta::define_cmd_args "define_power_switch_cell" {-name <name> \
                                                 -control <control_pin> \
                                                 [-acknowledge <acknowledge_pin>] \
                                                 -power_switchable <power_switchable_pin> \
                                                 -power <power_pin> \
                                                 -ground <ground_pin> }

proc define_power_switch_cell {args} {
  sta::parse_key_args "define_power_switch_cell" args \
    keys {-name -control -acknowledge -power_switchable -power -ground} flags {}

  sta::check_argc_eq0 "define_power_switch_cell" $args

  pdn::check_design_state "define_power_switch_cell"

  if {![info exists keys(-name)]} {
    utl::error PDN 1183 "The -name argument is required."
  } else {
    set master [[ord::get_db] findMaster $keys(-name)]
    if { $master == "NULL" } {
      utl::error PDN 1046 "Unable to find power switch cell master: $keys(-name)"
    }
  }

  if {![info exists keys(-control)]} {
    utl::error PDN 1184 "The -control argument is required."
  } else {
    set control [pdn::get_mterm $master $keys(-control)]
  }

  set acknowledge "NULL"
  if {[info exists keys(-acknowledge)]} {
    set acknowledge [pdn::get_mterm $master $keys(-acknowledge)]
  }

  if {![info exists keys(-power_switchable)]} {
    utl::error PDN 1186 "The -power_switchable argument is required."
  } else {
    set power_switchable [pdn::get_mterm $master $keys(-power_switchable)]
  }

  if {![info exists keys(-power)]} {
    utl::error PDN 1187 "The -power argument is required."
  } else {
    set power [pdn::get_mterm $master $keys(-power)]
  }

  if {![info exists keys(-ground)]} {
    utl::error PDN 1188 "The -ground argument is required."
  } else {
    set ground [pdn::get_mterm $master $keys(-ground)]
  }

  pdn::make_switched_power_cell \
    $master \
    $control \
    $acknowledge \
    $power_switchable \
    $power \
    $ground
}

sta::define_cmd_args "add_pdn_stripe" {[-grid grid_name] \
                                       -layer layer_name \
                                       [-width width_value] \
                                       [-followpins] \
                                       [-extend_to_core_ring] \
                                       [-pitch pitch_value] \
                                       [-spacing spacing_value] \
                                       [-offset offset_value] \
                                       [-starts_with (POWER|GROUND)]
                                       [-extend_to_boundary] \
                                       [-snap_to_grid] \
                                       [-number_of_straps count] \
                                       [-nets list_of_nets]
}

proc add_pdn_stripe {args} {
  sta::parse_key_args "add_pdn_stripe" args \
    keys {-grid -layer -width -pitch -spacing -offset -starts_with -number_of_straps -nets} \
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

  set nets {}
  if {[info exists keys(-nets)]} {
    foreach net_name $keys(-nets) {
      set net [[ord::get_db_block] findNet $net_name]
      if {$net == "NULL"} {
        utl::error PDN 225 "Unable to find net $net_name."
      }
      lappend nets $net
    }
  }

  set layer [pdn::get_layer $keys(-layer)]
  set width [ord::microns_to_dbu $width]
  set pitch [ord::microns_to_dbu $pitch]
  set spacing [ord::microns_to_dbu $spacing]
  set offset [ord::microns_to_dbu $offset]

  set extend "Core"
  if {[info exists flags(-extend_to_core_ring)] && [info exists flags(-extend_to_boundary)]} {
    utl::error PDN 1010 "Options -extend_to_core_ring and\
      -extend_to_boundary are mutually exclusive."
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
    pdn::make_strap \
      $grid \
      $layer \
      $width \
      $spacing \
      $pitch \
      $offset \
      $number_of_straps \
      [info exists flags(-snap_to_grid)] \
      $use_grid_power_order \
      $start_with_power \
      $extend \
      $nets
  }
}

sta::define_cmd_args "add_pdn_ring" {[-grid grid_name] \
                                     [-layers list_of_2_layer_names] \
                                     [-widths (width_value|list_of_width_values)] \
                                     [-spacings (spacing_value|list_of_spacing_values)] \
                                     [-core_offsets (offset_value|list_of_offset_values)] \
                                     [-pad_offsets (offset_value|list_of_offset_values)] \
                                     [-connect_to_pad_layers layers] \
                                     [-power_pads list_of_pwr_pads] \
                                     [-ground_pads list_of_gnd_pads] \
                                     [-nets list_of_nets] \
                                     [-starts_with (POWER|GROUND)] \
                                     [-add_connect] \
                                     [-extend_to_boundary] \
                                     [-connect_to_pads]
                                     }

proc add_pdn_ring {args} {
  sta::parse_key_args "add_pdn_ring" args \
    keys {-grid -layers -widths -spacings -core_offsets -pad_offsets -connect_to_pad_layers \
      -power_pads -ground_pads -nets -starts_with} \
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

  set layers_len [llength $keys(-layers)]
  if {$layers_len > 2} {
    utl::error PDN 1012 "Expecting a list of 1 or 2 elements for -layers option of add_pdn_ring\
      command, found ${layers_len}."
  }
  set layers $keys(-layers)
  if {$layers_len == 1} {
    set layers "$layers $layers"
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

  set nets {}
  if {[info exists keys(-nets)]} {
    foreach net_name $keys(-nets) {
      set net [[ord::get_db_block] findNet $net_name]
      if {$net == "NULL"} {
        utl::error PDN 230 "Unable to find net $net_name."
      }
      lappend nets $net
    }
  }

  set use_grid_power_order 1
  set start_with_power 0
  if {[info exists keys(-starts_with)]} {
    set use_grid_power_order 0
    set start_with_power [pdn::get_starts_with $keys(-starts_with)]
  }

  set l0 [pdn::get_layer [lindex $layers 0]]
  set l1 [pdn::get_layer [lindex $layers 1]]
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

  pdn::make_ring \
    $grid \
    $l0 \
    [lindex $widths 0] \
    [lindex $spacings 0] \
    $l1 \
    [lindex $widths 1] \
    [lindex $spacings 1] \
    $use_grid_power_order \
    $start_with_power \
    {*}$core_offsets \
    {*}$pad_offsets \
    [info exists flags(-extend_to_boundary)] \
    $connect_to_pad_layers \
    $nets

  if {[info exists flags(-add_connect)]} {
    add_pdn_connect -grid $grid -layers $keys(-layers)
  }
}

sta::define_cmd_args "add_pdn_connect" {[-grid grid_name] \
                                        -layers list_of_2_layers \
                                        [-cut_pitch pitch_value] \
                                        [-fixed_vias list_of_vias] \
                                        [-dont_use_vias list_of_vias]
                                        [-max_rows rows] \
                                        [-max_columns columns] \
                                        [-ongrid ongrid_layers] \
                                        [-split_cuts split_cuts_mapping]
}

proc add_pdn_connect {args} {
  sta::parse_key_args "add_pdn_connect" args \
    keys {-grid -layers -cut_pitch -fixed_vias -max_rows -max_columns -ongrid -split_cuts \
      -dont_use_vias} \
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
    [ord::microns_to_dbu [lindex $cut_pitch 0]] \
    [ord::microns_to_dbu [lindex $cut_pitch 1]]
  ]

  set grid ""
  if {[info exists keys(-grid)]} {
    set grid $keys(-grid)
  }

  set max_rows 0
  if {[info exists keys(-max_rows)]} {
    set max_rows $keys(-max_rows)
  }
  set max_columns 0
  if {[info exists keys(-max_columns)]} {
    set max_columns $keys(-max_columns)
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

  set ongrid {}
  if {[info exists keys(-ongrid)]} {
    foreach l $keys(-ongrid) {
      lappend ongrid [pdn::get_layer $l]
    }
  }

  set split_cuts_layers {}
  set split_cuts_pitches {}
  if {[info exists keys(-split_cuts)]} {
    foreach {l pitch} $keys(-split_cuts) {
      lappend split_cuts_layers [pdn::get_layer $l]
      lappend split_cuts_pitches [ord::microns_to_dbu $pitch]
    }
  }

  set dont_use ""
  if {[info exists keys(-dont_use_vias)]} {
    set dont_use $keys(-dont_use_vias)
  }

  pdn::make_connect \
    $grid \
    $l0 \
    $l1 \
    {*}$cut_pitch \
    $fixed_generate_vias \
    $fixed_tech_vias \
    $max_rows \
    $max_columns \
    $ongrid \
    $split_cuts_layers \
    $split_cuts_pitches \
    $dont_use
}

sta::define_cmd_args "add_sroute_connect" {[-net net] \
                                           [-outerNet outerNet] \
                                           -layers list_of_2_layers \
                                           -cut_pitch pitch_value \
                                           [-fixed_vias list_of_vias] \
                                           [-max_rows rows] \
                                           [-max_columns columns] \
                                           [-metalwidths metalwidths] \
                                           [-metalspaces metalspaces] \
                                           [-ongrid ongrid_layers] \
                                           [-insts inst]
}

proc add_sroute_connect {args} {
  sta::parse_key_args "add_sroute_connect" args \
    keys {-net -outerNet -layers -cut_pitch -fixed_vias -max_rows -max_columns -metalwidths \
      -metalspaces -ongrid -insts} \
    flags {}

  set l0 [pdn::get_layer [lindex $keys(-layers) 0]]
  set l1 [pdn::get_layer [lindex $keys(-layers) 1]]

  set cut_pitch_x 0
  set cut_pitch_y 0
  set cut_pitch_x [lindex $keys(-cut_pitch) 0]
  set cut_pitch_y [lindex $keys(-cut_pitch) 1]

  set net ""
  if {[info exists keys(-net)]} {
    set net $keys(-net)
  }

  set outerNet ""
  if {[info exists keys(-outerNet)]} {
    set outerNet $keys(-outerNet)
  }

  set max_rows 10
  if {[info exists keys(-max_rows)]} {
    set max_rows $keys(-max_rows)
  }
  set max_columns 10
  if {[info exists keys(-max_columns)]} {
    set max_columns $keys(-max_columns)
  }

  set fixed_generate_vias {}
  set fixed_tech_vias {}
  if {[info exists keys(-fixed_vias)]} {
    foreach via $keys(-fixed_vias) {
      set tech_via [[ord::get_db_tech] findVia $via]
      set generate_via [[ord::get_db_tech] findViaGenerateRule $via]
      if {$tech_via != "NULL"} {
        lappend fixed_tech_vias $tech_via
      }
      if {$generate_via != "NULL"} {
        lappend fixed_generate_vias $generate_via
      }
    }
  }

  set ongrid {}
  if {[info exists keys(-ongrid)]} {
    foreach l $keys(-ongrid) {
      lappend ongrid [pdn::get_layer $l]
    }
  }

  set insts {}
  if {[info exists keys(-insts)]} {
    foreach inst $keys(-insts) {
      set instance [[ord::get_db_block] findInst $inst]
      if {$instance != "NULL"} {
        lappend insts $instance
      }
    }
  }

  set metalwidths {}
  if {[info exists keys(-metalwidths)]} {
    foreach l $keys(-metalwidths) {
      lappend metalwidths $l
    }
  }

  set metalspaces {}
  if {[info exists keys(-metalspaces)]} {
    foreach l $keys(-metalspaces) {
      lappend metalspaces $l
    }
  }

  pdn::createSrouteWires \
    $net \
    $outerNet \
    $l0 \
    $l1 \
    $cut_pitch_x \
    $cut_pitch_y \
    $fixed_generate_vias \
    $fixed_tech_vias \
    $max_rows \
    $max_columns \
    $ongrid \
    $metalwidths \
    $metalspaces \
    $insts
}

sta::define_cmd_args "repair_pdn_vias" {[-net net_name] \
                                        -all
}
proc repair_pdn_vias { args } {
  sta::parse_key_args "repair_pdn_vias" args \
    keys {-net} \
    flags {-all}

  sta::check_argc_eq0 "repair_pdn_vias" $args
  pdn::check_design_state "repair_pdn_vias"

  if {[info exists keys(-net)] && [info exists flags(-all)]} {
    utl::error PDN 1191 "Cannot use both -net and -all arguments."
  }
  if {![info exists keys(-net)] && ![info exists flags(-all)]} {
    utl::error PDN 1192 "Must use either -net or -all arguments."
  }

  set nets []
  if {[info exists keys(-net)]} {
    set net [[ord::get_db_block] findNet $keys(-net)]
    if {$net == "NULL"} {
      utl::error PDN 1190 "Unable to find net: $keys(-net)"
    }
    lappend nets $net
  }
  if {[info exists flags(-all)]} {
    foreach net [[ord::get_db_block] getNets] {
      if {[$net getSigType] == "POWER" || [$net getSigType] == "GROUND"} {
        lappend nets $net
      }
    }
  }

  pdn::repair_pdn_vias $nets
}

# conversion utility
sta::define_hidden_cmd_args "convert_pdn_config" { config_file }
proc convert_pdn_config { args } {
  sta::parse_key_args "convert_pdn_config" args \
    keys {} \
    flags {}

  sta::check_argc_eq1 "convert_pdn_config" $args

  pdn::check_design_state "convert_pdn_config"

  pdn::convert_config $args
}

namespace eval pdn {
proc name_cmp { obj1 obj2 } {
  set name1 [$obj1 getName]
  set name2 [$obj2 getName]
  if { $name1 < $name2 } {
    return -1
  } elseif { $name1 == $name2 } {
    return 0
  } else {
    return 1
  }
}

proc check_design_state { args } {
  if {[ord::get_db_block] == "NULL"} {
    utl::error PDN 1022 "Design must be loaded before calling $args."
  }
}

proc modify_voltage_domain_name { name } {
  if {$name == "CORE"} {
    return "Core"
  }
  return $name
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
    keys {-name -voltage_domains -pins -starts_with -obstructions -power_switch_cell \
      -power_control -power_control_network} \
    flags {};# checker off

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

  if {[has_grid $keys(-name)]} {
    utl::error PDN 1043 "Grid named \"$keys(-name)\" already defined."
  }

  set start_with_power 0
  if {[info exists keys(-starts_with)]} {
    set start_with_power [pdn::get_starts_with $keys(-starts_with)]
  }

  set pin_layers {}
  if {[info exists keys(-pins)]} {
    foreach pin $keys(-pins) {
      lappend pin_layers [pdn::get_layer $pin]
    }
  }

  set obstructions {}
  if {[info exists keys(-obstructions)]} {
    set obstructions [get_obstructions $keys(-obstructions)]
  }

  set power_cell "NULL"
  set power_control "NULL"
  if {[info exists keys(-power_switch_cell)]} {
    set power_cell [pdn::find_switched_power_cell $keys(-power_switch_cell)]
    if { $power_cell == "NULL" } {
      utl::error PDN 1048 "Switched power cell $keys(-power_switch_cell) is not defined."
    }

    if {![info exists keys(-power_control)]} {
      utl::error PDN 1045 "-power_control must be specified with -power_switch_cell"
    } else {
      set power_control [[ord::get_db_block] findNet $keys(-power_control)]
      if { $power_control == "NULL" } {
        utl::error PDN 1049 "Unable to find power control net: $keys(-power_control)"
      }
    }
  }

  set power_control_network "STAR"
  if {[info exists keys(-power_control_network)]} {
    set power_control_network $keys(-power_control_network)
  }

  foreach domain $domains {
    pdn::make_core_grid \
      $domain \
      $keys(-name) \
      $start_with_power \
      $pin_layers \
      $obstructions \
      $power_cell \
      $power_control \
      $power_control_network
  }
}

proc define_pdn_grid_existing { args } {
  sta::parse_key_args "define_pdn_grid" args \
    keys {-name -obstructions} \
    flags {-existing};# checker off

  sta::check_argc_eq0 "define_pdn_grid" $args
  pdn::check_design_state "define_pdn_grid"

  set name "existing_grid"
  if {[info exists keys(-name)]} {
    set name $keys(-name)
  }

  set obstructions {}
  if {[info exists keys(-obstructions)]} {
    set obstructions [get_obstructions $keys(-obstructions)]
  }

  pdn::make_existing_grid $name $obstructions
}

proc define_pdn_grid_macro { args } {
  sta::parse_key_args "define_pdn_grid" args \
    keys {-name -voltage_domains -orient -instances -cells -halo -pin_direction -starts_with \
      -obstructions} \
    flags {-macro -grid_over_pg_pins -grid_over_boundary -default -bump};# checker off

  sta::check_argc_eq0 "define_pdn_grid" $args
  pdn::check_design_state "define_pdn_grid"

  set pg_pins_to_boundary 1
  pdn::depricated keys -pin_direction
  if {[info exists flags(-grid_over_pg_pins)] && [info exists flags(-grid_over_boundary)]} {
    utl::error PDN 1026 "Options -grid_over_pg_pins and -grid_over_boundary are mutually exclusive."
  } elseif {[info exists flags(-grid_over_pg_pins)]} {
    set pg_pins_to_boundary 0
  }

  set exclusive_keys 0
  if {[info exists keys(-instances)]} {
    incr exclusive_keys
  }
  if {[info exists keys(-cells)]} {
    incr exclusive_keys
  }
  if {[info exists flags(-default)]} {
    incr exclusive_keys
  }
  if {$exclusive_keys > 1} {
    utl::error PDN 1027 "Options -instances, -cells, and -default are mutually exclusive."
  } elseif {![info exists keys(-instances)] &&
            ![info exists keys(-cells)] &&
            ![info exists flags(-default)]} {
    utl::error PDN 1028 "Either -instances, -cells, or -default must be specified."
  }
  set default_grid [info exists flags(-default)]
  if {$default_grid} {
    # set default pattern to .*
    set keys(-cells) ".*"
  }

  if {![info exists keys(-name)]} {
    utl::error PDN 1029 "-name is required"
  }
  if {[has_grid $keys(-name)]} {
    utl::error PDN 1044 "Grid named \"$keys(-name)\" already defined."
  }

  set start_with_power 0
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

  set obstructions {}
  if {[info exists keys(-obstructions)]} {
    set obstructions [get_obstructions $keys(-obstructions)]
  }

  set orients {}
  if {[info exists keys(-orient)]} {
    set orients [pdn::get_orientations $keys(-orient)]
  }

  set is_bump [info exists flags(-bump)]

  if {[info exists keys(-instances)]} {
    set insts {}
    foreach inst_pattern $keys(-instances) {
      set sub_insts [get_insts $inst_pattern]
      if {[llength $sub_insts] == 0} {
        utl::error PDN 1030 "Unable to find instance: $inst_pattern"
      }
      foreach inst $sub_insts {
        lappend insts $inst
      }
    }

    set insts [lsort -unique -command name_cmp $insts]
    foreach inst $insts {
      # must match orientation, if provided
      if {[match_orientation $orients [$inst getOrient]] != 0} {
        foreach domain $domains {
          pdn::make_instance_grid \
            $domain \
            $keys(-name) \
            $start_with_power \
            $inst \
            {*}$halo \
            $pg_pins_to_boundary \
            $default_grid \
            $obstructions \
            $is_bump
        }
      }
    }
  } else {
    set cells {}
    foreach cell_pattern $keys(-cells) {
      foreach cell [get_masters $cell_pattern] {
        # only add blocks
        if {![$cell isBlock]} {
          continue;
        }
        lappend cells $cell
      }
    }

    set cells [lsort -unique -command name_cmp $cells]
    foreach cell $cells {
      foreach inst [[ord::get_db_block] getInsts] {
        # inst must match cells
        if {[$inst getMaster] == $cell} {
          # must match orientation, if provided
          if {[match_orientation $orients [$inst getOrient]] != 0} {
            foreach domain $domains {
              pdn::make_instance_grid \
                $domain $keys(-name) \
                $start_with_power \
                $inst \
                {*}$halo \
                $pg_pins_to_boundary \
                $default_grid \
                $obstructions \
                $is_bump
            }
          }
        }
      }
    }
  }
}

proc get_voltage_domains { names } {
  set domains {}
  foreach name $names {
    set domain [pdn::find_domain [modify_voltage_domain_name $name]]

    if {$domain == "NULL"} {
      utl::error PDN 1032 "Unable to find $name domain."
    }
    lappend domains $domain
  }
  return $domains
}

proc match_orientation { orients orient } {
  if {[llength $orients] == 0} {
    return 1
  }

  if {[lsearch -exact $orients $orient] != -1} {
    return 1
  }

  return 0
}

proc get_insts { pattern } {
  set insts {}
  foreach inst [[ord::get_db_block] getInsts] {
    if {[regexp $pattern [$inst getName]] != 0} {
      lappend insts $inst
    }
  }
  return $insts
}

proc get_masters { pattern } {
  set masters {}
  foreach lib [[ord::get_db] getLibs] {
    foreach master [$lib getMasters] {
      if {[regexp $pattern [$master getName]] != 0} {
        lappend masters $master
      }
    }
  }
  return $masters
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

proc get_obstructions { obstruction } {
  set layers {}
  foreach l $obstruction {
    lappend layers [pdn::get_layer $l]
  }
  return $layers
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

proc get_mterm { master term } {
  set mterm [$master findMTerm $term]
  if { $mterm == "NULL" } {
    utl::error PDN 1047 "Unable to find $term on $master"
  }
  return $mterm
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
      set valid_orients [join $valid_orientations {, }]
      utl::error PDN 1036 "Invalid orientation $orient specified, must be one of ${valid_orients}."
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
  # reset PDNGEN, copies of initial values from pdngen tcl
  set pdngen::global_connections {}
  set pdngen::voltage_domains {
    CORE {
      primary_power VDD primary_ground VSS
    }
  }
  set pdngen::power_nets {}
  set pdngen::ground_nets {}
  set pdngen::design_data {}

  # Source configuration
  pdngen::source_config $config

  generate_global_connect_commands

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

  if {[dict exists $pdngen::design_data grid]} {
    if {[dict exists $pdngen::design_data grid stdcell]} {
      convert_header "standard cell grid"
      dict for {name spec} [dict get $pdngen::design_data grid stdcell] {
        set command [generate_grid_command $spec]

        puts "$command"

        generate_grid_commands $name $spec
      }
    }

    if {[dict exists $pdngen::design_data grid macro]} {
      convert_header "macro grids"

      set inst_grids {}
      set cell_grids {}
      set catch_all {}

      dict for {name spec} [dict get $pdngen::design_data grid macro] {
        if {[dict exists $spec instances]} {
          lappend inst_grids $name $spec
        } elseif {[dict exists $spec macro]} {
          lappend cell_grids $name $spec
        } else {
          lappend catch_all $name $spec
        }
      }

      # order matters, so ensure instances are assigned first, then cells, and then catch all
      set grids [list \
        {*}$inst_grids \
        {*}$cell_grids \
        {*}$catch_all]

      foreach {name spec} $grids {
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
        } elseif {[dict exists $spec macro]} {
          append command " -cells \{[dict get $spec macro]\}"
        } else {
          append command " -default"
        }
        if {[dict exists $spec grid_over_pg_pins]} {
          if {[dict get $spec grid_over_pg_pins] == 1} {
            append command " -grid_over_pg_pins"
          } else {
            append command " -grid_over_boundary"
          }
        } else {
          append command " -grid_over_boundary"
        }

        puts "$command"

        generate_grid_commands $name $spec
      }
    }
  }
}

proc generate_global_connect_commands { } {
  convert_header "global connections"
  # build global connections
  set global_conns $pdngen::default_global_connections
  dict for {net conn} $pdngen::global_connections {
    foreach con $conn {
      if {$con ni [dict get $global_conns $net]} {
        dict lappend global_conns $net $con
      }
    }
  }

  dict for {net net_conn} $global_conns {
    set extra_append ""
    if {[lsearch -exact [pdngen::get_pdn_power_nets] $net] != -1} {
      set extra_append " -power"
    } elseif {[lsearch -exact [pdngen::get_pdn_ground_nets] $net] != -1} {
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

  if {[llength $global_conns] > 0} {
    puts "global_connect"
  }
}

proc generate_grid_command { spec } {
  set command "define_pdn_grid"
  append command " -name \{[dict get $spec name]\}"
  append command " -voltage_domains \{[dict get $spec voltage_domains]\}"
  if {[dict exists $spec starts_with]} {
    append command " -starts_with \{[dict get $spec starts_with]\}"
  }
  if {[dict exists $spec obstructions]} {
    append command " -obstructions \{[dict get $spec obstructions]\}"
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
    set layers {}
    set widths {}
    set spacings {}
    set core_offsets {}
    set pad_offsets {}
    set connect_to_pad 0
    dict for {layer ring} [dict get $spec core_ring] {
      lappend layers $layer
      lappend widths [dict get $ring width]
      lappend spacings [dict get $ring spacing]
      if {[dict exists $ring core_offset]} {
        lappend core_offsets [dict get $ring core_offset]
      }
      if {[dict exists $ring pad_offset]} {
        lappend pad_offsets [dict get $ring pad_offset]
      }
    }

    if {[dict exists $spec pwr_pads] || [dict exists $spec gnd_pads]} {
      set connect_to_pad 1
    }

    set command "add_pdn_ring -grid \{$grid_name\}"

    append command " -layers \{$layers\}"
    append command " -widths \{$widths\}"
    append command " -spacings \{$spacings\}"
    if {[llength $pad_offsets] > 0} {
      append command " -pad_offsets \{$pad_offsets\}"
    } else {
      append command " -core_offsets \{$core_offsets\}"
    }
    if {$connect_to_pad} {
      append command " -connect_to_pads"
    }
    append command " -starts_with POWER"

    puts "$command"
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
      append command " -layers \{$l0 $l1\}"

      for {set c 2} {$c < [llength $connect]} {incr c} {
        if {[lindex $connect $c] == "fixed_vias"} {
          incr c
          append command " -fixed_vias \{[lindex $connect $c]\}"
        }
        if {[lindex $connect $c] == "cut_pitch"} {
          incr c
          append command " -cut_pitch \{[lindex $connect $c]\}"
        }
        if {[lindex $connect $c] == "constraints"} {
          incr c
          set constraints [lindex $connect $c]
          if {[dict exists $constraints max_rows]} {
            append command " -max_rows \{[dict get $constraints max_rows]\}"
          }
          if {[dict exists $constraints max_columns]} {
            append command " -max_columns \{[dict get $constraints max_columns]\}"
          }
          if {[dict exists $constraints ongrid]} {
            append command " -ongrid \{[dict get $constraints ongrid]\}"
          }
          if {[dict exists $constraints split_cuts]} {
            append command " -split_cuts \{[dict get $constraints split_cuts]\}"
          }
        }
      }

      puts "$command"
    }
  }
}

proc generate_strap_commands { grid_name layer strap } {
  set command "add_pdn_stripe -grid \{$grid_name\}"
  append command " -layer \{$layer\}"
  if {[dict exists $strap width]} {
    append command " -width \{[dict get $strap width]\}"
  }
  if {[dict exists $strap spacing]} {
    append command " -spacing \{[dict get $strap spacing]\}"
  }
  if {[dict exists $strap pitch]} {
    append command " -pitch \{[dict get $strap pitch]\}"
  }
  if {[dict exists $strap offset]} {
    append command " -offset \{[dict get $strap offset]\}"
  }
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

proc get_pdn_power_nets {} {
  if {[info vars ::power_nets] == ""} {
    return "VDD"
  } else {
    return $::power_nets
  }
}

proc get_pdn_ground_nets {} {
  if {[info vars ::ground_nets] == ""} {
    return "VSS"
  } else {
    return $::ground_nets
  }
}
# namespace pdngen
}