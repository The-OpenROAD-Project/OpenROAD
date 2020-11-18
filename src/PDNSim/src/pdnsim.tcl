sta::define_cmd_args "analyze_power_grid" { 
  [-vsrc vsrc_file ]
  [-outfile out_file]
  [-enable_em]
  [-em_outfile em_out_file]
  [-net net_name]
  }

proc analyze_power_grid { args } {
  sta::parse_key_args "analyze_power_grid" args \
    keys {-vsrc -outfile -em_outfile -net} flags {-enable_em}

  if { [info exists keys(-vsrc)] } {
    set vsrc_file $keys(-vsrc)
    if { [file readable $vsrc_file] } {
      pdnsim_import_vsrc_cfg_cmd $vsrc_file
    } else {
      ord::error "Cannot read $vsrc_file"
    }
  } else {
    ord::error "Key vsrc not defined."
  }
 
 if { [info exists keys(-net)] } {
    set net $keys(-net)
    pdnsim_set_power_net $net
  } else {
    ord::error "Key net name not specified"
  }


  #pdnsim_set_power_net $net
  if { [info exists keys(-outfile)] } {
    set out_file $keys(-outfile)
    pdnsim_import_out_file_cmd $out_file
  }
  set enable_em [info exists flags(-enable_em)]
  pdnsim_import_em_enable $enable_em
  if { [info exists keys(-em_outfile)]} {
    set em_out_file $keys(-em_outfile)
    if { $enable_em } {
      pdnsim_import_em_out_file_cmd $em_out_file
    } else {
      ord::error "EM outfile defined without EM enable flag. Add -enable_em."  
    }
  }


  if { [ord::db_has_rows] } {
    pdnsim_analyze_power_grid_cmd
  } else {
  	ord::error "No rows defined in design. Floorplan not defined. Use initialize_floorplan to add rows" 
  }
}

sta::define_cmd_args "check_power_grid" { 
  [-vsrc vsrc_file ]
  [-net power_net]}

proc check_power_grid { args } {
  sta::parse_key_args "check_power_grid" args \
    keys {-net} flags {}

  if { [info exists keys(-vsrc)] } {
    set vsrc_file $keys(-vsrc)
    if { [file readable $vsrc_file] } {
      pdnsim_import_vsrc_cfg_cmd $vsrc_file
    } else {
      ord::error "Cannot read $vsrc_file"
    }
  } 
 
 if { [info exists keys(-net)] } {
    set net $keys(-net)
    pdnsim_set_power_net $net
  } else {
    ord::error "Key net name not specified"
  }

  if { [ord::db_has_rows] } {
  	set res [pdnsim_check_connectivity_cmd]
  	return $res
  } else {
  	ord::error "No rows defined in design. Use initialize_floorplan to add rows" 
  }
}

sta::define_cmd_args "write_pg_spice" { 
  [-vsrc vsrc_file ]
  [-outfile out_file]
  [-net net_name]}

proc write_pg_spice { args } {
  sta::parse_key_args "write_pg_spice" args \
    keys {-vsrc -outfile -net} flags {}

  if { [info exists keys(-vsrc)] } {
    set vsrc_file $keys(-vsrc)
    if { [file readable $vsrc_file] } {
      pdnsim_import_vsrc_cfg_cmd $vsrc_file
    } else {
      ord::error "Cannot read $vsrc_file"
    }
  } else {
    ord::error "key vsrc not defined."
  }

  if { [info exists keys(-outfile)] } {
    set out_file $keys(-outfile)
     pdnsim_import_spice_out_file_cmd $out_file
  }
  
  if { [info exists keys(-net)] } {
    set net $keys(-net)
    pdnsim_set_power_net $net
  } else {
    ord::error "Key net name not specified"
  }

  
  if { [ord::db_has_rows] } {
    pdnsim_write_pg_spice_cmd
  } else {
  	ord::error "No rows defined in design. Use initialize_floorplan to add rows and construct PDN" 
  }
}
