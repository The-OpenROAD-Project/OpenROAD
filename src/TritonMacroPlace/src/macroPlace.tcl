
sta::define_cmd_args "macro_placement" {
  [-global_config global_config_file]\
  [-local_config local_config_file]}

proc macro_placement { args } {
  sta::parse_key_args "macro_placement" args \
    keys {-global_config -local_config -fence_region} flags {-die_area}

  if { ![info exists keys(-global_config)] } {
    puts "Error: -global_config must exist."
    return
  } else {
    set global_config_file $keys(-global_config)
    if { [file readable $global_config_file] } {
      set_macro_place_global_config_cmd $global_config_file
    } else {
      puts "Warning: cannot read $global_config_file"
    }
  }

  if { [info exists keys(-local_config)] } {
    set local_config_file $keys(-local_config)
    if { [file readable $local_config_file] } {
      set_macro_place_local_config_cmd $local_config_file
    } else {
      puts "Warning: cannot read $local_config_file"
    }
  }

  if { [info exists flags(-die_area)] } {
    if { [info exists flags(-fence_region)] } {
      puts "Warning: flag -die_area and -fence_region set."
      puts "         TritonMP will ignore -fence_region flag."
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

    set_macro_place_fence_region_cmd $dieLx $dieLy $dieUx $dieUy
  }

  if { [info exists keys(-fence_region)] } {
    set fence_region $keys(-fence_region)
    
    # get dbu 
    set db [ord::get_db]
    set block [[$db getChip] getBlock]
    set die_area [$block getDieArea]
    set dbu [$block getDbUnitsPerMicron]

    set lx [lindex $fence_region 0] 
    set ly [lindex $fence_region 1]
    set ux [lindex $fence_region 2]
    set uy [lindex $fence_region 3]
    
    # note that unit is micron
    set dieLx [expr double([$die_area xMin]) / $dbu]
    set dieLy [expr double([$die_area yMin]) / $dbu]
    set dieUx [expr double([$die_area xMax]) / $dbu]
    set dieUy [expr double([$die_area yMax]) / $dbu]

    if { $lx < $dieLx } {
      puts "Warning: fence_region lx is lower than dieLx."
      puts "         modify lx as $dieLx"
      set lx $dieLx
    }
    if { $ly < $dieLy } {
      puts "Warning: fence_region ly is lower than dieLy."
      puts "         modify ly as $dieLy"
      set ly $dieLy
    }
    if { $ux > $dieUx } {
      puts "Warning: fence_region ux exceeds dieUx."
      puts "         modify ux as $dieUx"
      set ux $dieUx
    }
    if { $uy > $dieUy } {
      puts "Warning: fence_region uy exceeds dieUy."
      puts "         modify uy as $dieUy"
      set uy $dieUy
    }

    set_macro_place_fence_region_cmd $lx $ly $ux $uy
  }
  
  if { [ord::db_has_rows] } {
    place_macros_cmd
  } else {
    puts "Error: no rows defined in design. Use initialize_floorplan to add rows."
  }
}
