
sta::define_cmd_args "macro_placement" {
  [-global_config global_config_file]\
  [-local_config local_config_file]}

proc macro_placement { args } {
  sta::parse_key_args "macro_placement" args \
    keys {-global_config -local_config -fence_region} flags {-die_area}

  if { ![info exists keys(-global_config)] } {
    utl::error "MPL" 81 "missing -global_config argument."
    return
  } else {
    set global_config_file $keys(-global_config)
    if { [file readable $global_config_file] } {
      mpl::set_macro_place_global_config_cmd $global_config_file
    } else {
      utl::warn "MPL" 82 "cannot read $global_config_file"
    }
  }

  if { [info exists keys(-local_config)] } {
    set local_config_file $keys(-local_config)
    if { [file readable $local_config_file] } {
      mpl::set_macro_place_local_config_cmd $local_config_file
    } else {
      utl::warn "MPL" 83 "cannot read $local_config_file"
    }
  }

  if { [info exists flags(-die_area)] } {
    if { [info exists flags(-fence_region)] } {
      utl::warn "MPL" 84 "both -die_area and -fence_region arguments, using -die_area."
    }

    # get dieArea from odb and set fence region 
    set db [ord::get_db]
    set block [[$db getChip] getBlock]
    set die_area [$block getDieArea]
    set dbu [$block getDbUnitsPerMicron]

    # note that unit is micron
    set dieLx [expr double([$die_area xMin]) / $dbu]
    set dieLy [expr double([$die_area yMin]) / $dbu]
    set dieUx [expr double([$die_area xMax]) / $dbu]
    set dieUy [expr double([$die_area yMax]) / $dbu]

    mpl::set_macro_place_fence_region_cmd $dieLx $dieLy $dieUx $dieUy
  }

  if { [info exists keys(-fence_region)] } {
    set fence_region $keys(-fence_region)
    
    # get dbu 
    set db [ord::get_db]
    set block [[$db getChip] getBlock]
    set die_area [$block getDieArea]
    set dbu [$block getDbUnitsPerMicron]

    lassign $fence_region lx ly ux uy 
    
    # note that unit is micron
    set dieLx [expr double([$die_area xMin]) / $dbu]
    set dieLy [expr double([$die_area yMin]) / $dbu]
    set dieUx [expr double([$die_area xMax]) / $dbu]
    set dieUy [expr double([$die_area yMax]) / $dbu]

    if { $lx < $dieLx } {
      utl::warn "MPL" 85 "fence_region left x is less than die left x."
      set lx $dieLx
    }
    if { $ly < $dieLy } {
      utl::warn "MPL" 86 "fence_region bottom y is less than die bottom y."
      set ly $dieLy
    }
    if { $ux > $dieUx } {
      utl::warn "MPL" 87 "fence_region right x is greater than die right x."
      set ux $dieUx
    }
    if { $uy > $dieUy } {
      utl::warn "MPL" 88 "fence_region top y is greater than die top y."
      set uy $dieUy
    }

    mpl::set_macro_place_fence_region_cmd $lx $ly $ux $uy
  }
  
  if { [ord::db_has_rows] } {
    mpl::place_macros_cmd
  } else {
    utl::error "MPL" 89 "No rows found. Use initialize_floorplan to add rows."
  }
}
