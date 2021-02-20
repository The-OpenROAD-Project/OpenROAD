sta::define_cmd_args "analyze_power_grid" { 
  [-vsrc vsrc_file ]
  [-outfile out_file]
  [-enable_em]
  [-em_outfile em_out_file]
  [-net net_name]
  [-dx bump_pitch_x]
  [-dy bump_pitch_y]
  }

proc analyze_power_grid { args } {
  sta::parse_key_args "analyze_power_grid" args \
    keys {-vsrc -outfile -em_outfile -net -dx -dy} flags {-enable_em}
  if { [info exists keys(-vsrc)] } {
    set vsrc_file $keys(-vsrc)
    if { [file readable $vsrc_file] } {
      psm::import_vsrc_cfg_cmd $vsrc_file
    } else {
      utl::error PSM 53 "Cannot read $vsrc_file."
    }
  } 
  if { [info exists keys(-net)] } {
    set net $keys(-net)
    psm::set_power_net $net
  } else {
    utl::error PSM 54 "Argument -net not specified."
  }
  if { [info exists keys(-dx)] } {
    set bump_pitch_x $keys(-dx)
    psm::set_bump_pitch_x $bump_pitch_x
  }
  if { [info exists keys(-dy)] } {
    set bump_pitch_y $keys(-dy)
    psm::set_bump_pitch_y $bump_pitch_y
  }
  if { [info exists keys(-outfile)] } {
    set out_file $keys(-outfile)
    psm::import_out_file_cmd $out_file
  }
  set enable_em [info exists flags(-enable_em)]
  psm::import_em_enable $enable_em
  if { [info exists keys(-em_outfile)]} {
    set em_out_file $keys(-em_outfile)
    if { $enable_em } {
      psm::import_em_out_file_cmd $em_out_file
    } else {
      utl::error PSM 55 "EM outfile defined without EM enable flag. Add -enable_em."  
    }
  }
  if { [ord::db_has_rows] } {
    psm::analyze_power_grid_cmd
  } else {
    utl::error PSM 56 "No rows defined in design. Floorplan not defined. Use initialize_floorplan to add rows." 
  }
}

sta::define_cmd_args "check_power_grid" { 
  [-net power_net]}

proc check_power_grid { args } {
  sta::parse_key_args "check_power_grid" args \
    keys {-net} flags {}
  if { [info exists keys(-net)] } {
     set net $keys(-net)
     psm::set_power_net $net
  } else {
     utl::error PSM 57 "Argument -net not specified."
  }
  if { [ord::db_has_rows] } {
    set res [psm::check_connectivity_cmd]
    return $res
  } else {
    utl::error PSM 58 "No rows defined in design. Use initialize_floorplan to add rows." 
  }
}

sta::define_cmd_args "write_pg_spice" { 
  [-vsrc vsrc_file ]
  [-outfile out_file]
  [-net net_name]
  [-dx bump_pitch_x]
  [-dy bump_pitch_y]
  }

proc write_pg_spice { args } {
  sta::parse_key_args "write_pg_spice" args \
    keys {-vsrc -outfile -net -dx -dy} flags {}
  if { [info exists keys(-vsrc)] } {
    set vsrc_file $keys(-vsrc)
    if { [file readable $vsrc_file] } {
      psm::import_vsrc_cfg_cmd $vsrc_file
    } else {
      utl::error PSM 59 "Cannot read $vsrc_file."
    }
  } 
  if { [info exists keys(-outfile)] } {
    set out_file $keys(-outfile)
     psm::import_spice_out_file_cmd $out_file
  }
  if { [info exists keys(-net)] } {
    set net $keys(-net)
    psm::set_power_net $net
  } else {
    utl::error PSM 60 "Argument -net not specified."
  }
  if { [info exists keys(-dx)] } {
    set bump_pitch_x $keys(-dx)
    psm::set_bump_pitch_x $bump_pitch_x
  }
  if { [info exists keys(-dy)] } {
    set bump_pitch_y $keys(-dy)
    psm::set_bump_pitch_y $bump_pitch_y
  }

  if { [ord::db_has_rows] } {
    psm::write_pg_spice_cmd
  } else {
    utl::error PSM 61 "No rows defined in design. Use initialize_floorplan to add rows and construct PDN." 
  }
}

sta::define_cmd_args "set_pdnsim_net_voltage" { 
  [-net net_name]
  [-voltage volt]}

proc set_pdnsim_net_voltage { args } {
  sta::parse_key_args "set_pdnsim_net_voltage" args \
    keys {-net -voltage} flags {}
  if { [info exists keys(-net)] && [info exists keys(-voltage)] } {
    set net $keys(-net)
    set voltage $keys(-voltage)
    psm::set_net_voltage_cmd $net $voltage
  } else {
    utl::error PSM 62 "Argument -net or -voltage not specified. Please specifiy both -net and -voltage arguments."
  }
}



