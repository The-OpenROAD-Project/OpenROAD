# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

sta::define_cmd_args "check_power_grid" {
  -net power_net
  [-error_file error_file]
  [-floorplanning]
  [-dont_require_terminals]
}

proc check_power_grid { args } {
  sta::parse_key_args "check_power_grid" args \
    keys {-net -error_file} \
    flags {-floorplanning -dont_require_terminals}

  if { ![info exists keys(-net)] } {
    utl::error PSM 57 "Argument -net not specified."
  }

  set error_file ""
  if { [info exists keys(-error_file)] } {
    set error_file $keys(-error_file)
  }

  set floorplanning [info exists flags(-floorplanning)]
  set dont_require_bterm [info exists flags(-dont_require_terminals)]

  psm::check_connectivity_cmd \
    [psm::find_net $keys(-net)] \
    $floorplanning \
    $error_file \
    $dont_require_bterm
}

sta::define_cmd_args "analyze_power_grid" {
  -net net_name
  [-corner corner]
  [-error_file error_file]
  [-voltage_file voltage_file]
  [-enable_em]
  [-em_outfile em_file]
  [-vsrc voltage_source_file]
  [-source_type FULL|BUMPS|STRAPS]
  [-allow_reuse]
}

proc analyze_power_grid { args } {
  sta::parse_key_args "analyze_power_grid" args \
    keys {-net -corner -voltage_file -error_file -em_outfile -vsrc \
      -source_type} \
    flags {-enable_em -allow_reuse}
  if { ![info exists keys(-net)] } {
    utl::error PSM 58 "Argument -net not specified."
  }

  set error_file ""
  if { [info exists keys(-error_file)] } {
    set error_file $keys(-error_file)
  }

  set voltage_file ""
  if { [info exists keys(-voltage_file)] } {
    set voltage_file $keys(-voltage_file)
  }

  set voltage_source_file ""
  if { [info exists keys(-vsrc)] } {
    set voltage_source_file $keys(-vsrc)
  }

  set source_type "BUMPS"
  if { [info exists keys(-source_type)] } {
    set source_type $keys(-source_type)
  }

  set enable_em [info exists flags(-enable_em)]
  set em_file ""
  if { [info exists keys(-em_outfile)] } {
    set em_file $keys(-em_outfile)
    if { !$enable_em } {
      utl::error PSM 55 "EM file cannot be specified without enabling em analysis."
    }
  }

  psm::analyze_power_grid_cmd \
    [psm::find_net $keys(-net)] \
    [sta::parse_scene_or_default keys] \
    $source_type \
    $error_file \
    [info exists flags(-allow_reuse)] \
    $enable_em \
    $em_file \
    $voltage_file \
    $voltage_source_file
}

sta::define_cmd_args "insert_decap" { -target_cap target_cap\
                                      -cells cell_info\
                                      [-net net_name]\
                                    }

proc insert_decap { args } {
  sta::parse_key_args "insert_decap" args \
    keys {-target_cap -cells -net} flags {}

  set target_cap 0.0
  if { [info exists keys(-target_cap)] } {
    set target_cap $keys(-target_cap)
    sta::check_positive_float "-target_cap" $target_cap
    # F/m
    set target_cap [expr [sta::capacitance_ui_sta $target_cap] / [sta::distance_ui_sta 1.0]]
  }

  # Check even size
  set cells_and_decap $keys(-cells)
  if { [llength $cells_and_decap] % 2 != 0 } {
    utl::error PSM 181 "-cells must be a list of cell and decap pairs"
  }

  # Add decap cells
  set db [ord::get_db]
  foreach {cell_name decap} $cells_and_decap {
    set decap_value $decap
    sta::check_positive_float "-cells" $decap_value
    # F/m
    set decap_value [expr [sta::capacitance_ui_sta $decap_value] / [sta::distance_ui_sta 1.0]]
    # Find master with cell_name
    set matched 0
    foreach lib [$db getLibs] {
      foreach master [$lib getMasters] {
        set master_name [$master getConstName]
        if { [string match $cell_name $master_name] } {
          psm::add_decap_master $master $decap_value
          set matched 1
        }
      }
    }
    if { !$matched } {
      utl::warn "PSM" 280 "$cell_name did not match any masters."
    }
  }
  # Get net name
  set net_name ""
  if { [info exists keys(-net)] } {
    set net_name $keys(-net)
  }

  # Insert decap cells
  psm::insert_decap_cmd $target_cap $net_name
}

sta::define_cmd_args "write_pg_spice" {
  -net net_name
  [-vsrc vsrc_file]
  [-corner corner]
  [-source_type FULL|BUMPS|STRAPS]
  spice_file
  }

proc write_pg_spice { args } {
  sta::parse_key_args "write_pg_spice" args \
    keys {-vsrc -net -corner -source_type} flags {}
  sta::check_argc_eq1 "write_pg_spice" $args

  if { ![info exists keys(-net)] } {
    utl::error PSM 59 "Argument -net not specified."
  }

  set voltage_source_file ""
  if { [info exists keys(-vsrc)] } {
    set voltage_source_file $keys(-vsrc)
  }

  set source_type "BUMPS"
  if { [info exists keys(-source_type)] } {
    set source_type $keys(-source_type)
  }

  psm::write_spice_file_cmd \
    [psm::find_net $keys(-net)] \
    [sta::parse_scene_or_default keys] \
    $source_type \
    [lindex $args 0] \
    $voltage_source_file
}

sta::define_cmd_args "set_pdnsim_net_voltage" {
  -net net_name
  -voltage volt
  [-corner corner]}

proc set_pdnsim_net_voltage { args } {
  sta::parse_key_args "set_pdnsim_net_voltage" args \
    keys {-net -corner -voltage} flags {}
  if { [info exists keys(-net)] && [info exists keys(-voltage)] } {
    set net [psm::find_net $keys(-net)]
    set voltage $keys(-voltage)
    set corner [sta::parse_scene_or_default keys]
    psm::set_net_voltage_cmd $net $corner $voltage
  } else {
    utl::error PSM 62 "Argument -net or -voltage not specified.\
      Please specify both -net and -voltage arguments."
  }
}

sta::define_cmd_args "set_pdnsim_inst_power" {
  -inst instance
  -power power
  [-corner corner]}

proc set_pdnsim_inst_power { args } {
  sta::parse_key_args "set_pdnsim_inst_power" args \
    keys {-inst -corner -power} flags {}
  if { [info exists keys(-inst)] && [info exists keys(-power)] } {
    set inst [psm::find_inst $keys(-inst)]
    set power $keys(-power)
    set corner [sta::parse_scene_or_default keys]
    psm::set_inst_power $inst $corner $power
  } else {
    utl::error PSM 63 "Argument -inst or -power not specified.\
      Please specify both -inst and -power arguments."
  }
}

sta::define_cmd_args "set_pdnsim_source_settings" {
  [-bump_dx pitch]
  [-bump_dy pitch]
  [-bump_size size]
  [-bump_interval interval]
  [-strap_track_pitch pitch]
  [-external_resistance resistance]
}

proc set_pdnsim_source_settings { args } {
  sta::parse_key_args "set_pdnsim_source_settings" args \
    keys {-bump_dx -bump_dy -bump_size -bump_interval -strap_track_pitch -external_resistance} \
    flags {}

  set dx 0
  if { [info exists keys(-bump_dx)] } {
    set dx $keys(-bump_dx)
  }
  set dy 0
  if { [info exists keys(-bump_dy)] } {
    set dy $keys(-bump_dy)
  }
  set size 0
  if { [info exists keys(-bump_size)] } {
    set size $keys(-bump_size)
  }
  set interval 0
  if { [info exists keys(-bump_interval)] } {
    set interval $keys(-bump_interval)
  }

  set track_pitch 0
  if { [info exists keys(-strap_track_pitch)] } {
    set track_pitch $keys(-strap_track_pitch)
  }

  set resistance 0
  if { [info exists keys(-external_resistance)] } {
    set resistance $keys(-external_resistance)
  }

  psm::set_source_settings $dx $dy $size $interval $track_pitch $resistance
}

namespace eval psm {
proc find_net { net_name } {
  set net [[ord::get_db_block] findNet $net_name]
  if { $net == "NULL" } {
    utl::error PSM 28 "Cannot find net $net_name in the design."
  }
  return $net
}

proc find_inst { inst_name } {
  set inst [[ord::get_db_block] findInst $inst_name]
  if { $inst == "NULL" } {
    utl::error PSM 29 "Cannot find instance $inst_name in the design."
  }
  return $inst
}
}
