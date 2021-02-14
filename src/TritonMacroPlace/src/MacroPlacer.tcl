
sta::define_cmd_args "macro_placement" {
  -halo {vertical_width horizontal_width} \
    -channel {vertical_width horizontal_width}\
    [-fence_region {lx ly ux uy}]}

proc macro_placement { args } {
  sta::parse_key_args "macro_placement" args \
    keys {-channel -halo -fence_region -global_config -local_config} flags {}

  if { [info exists keys(-halo)] } {
    lasssign $keys(-halo) halo_v halo_h
    sta::check_positive_float "-halo vertical" $halo_v
    sta::check_positive_float "-halo horizontal" $halo_h
    mpl::set_halo $halo_v $halo_h
  }

  if { [info exists keys(-channel)] } {
    lasssign $keys(-channel) channel_v channel_h
    sta::check_positive_float "-channel vertical" $channel_v
    sta::check_positive_float "-channel horizontal" $channel_h
    mpl::set_channel $channel_v $channel_h
  }

  set block [ord::get_db_block]
  set die_area [$block getDieArea]
  # note that unit is micron
  set dieLx [ord::dbu_to_microns [$die_area xMin]]
  set dieLy [ord::dbu_to_microns [$die_area yMin]]
  set dieUx [ord::dbu_to_microns [$die_area xMax]]
  set dieUy [ord::dbu_to_microns [$die_area yMax]]
  
  if { [info exists keys(-fence_region)] } {
    lassign $keys(-fence_region) lx ly ux uy 
    
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
    mpl::set_fence_region $lx $ly $ux $uy
  } else {
    mpl::set_fence_region $dieLx $dieLy $dieUx $dieUy
  }
  
  if { [info exists keys(-global_config)] } {
    #utl::warn "MPL" 81 "macro place -global_config deprecated. Use -channel, -halo arguments."
    set global_config_file $keys(-global_config)
    if { [file readable $global_config_file] } {
      mpl::set_global_config $global_config_file
    } else {
      utl::warn "MPL" 82 "cannot read $global_config_file"
    }
  }

  if { [info exists keys(-local_config)] } {
    utl::warn "MPL" 90 "macro place -local_config deprecated."
    set local_config_file $keys(-local_config)
    if { [file readable $local_config_file] } {
      mpl::set_local_config $local_config_file
    } else {
      utl::warn "MPL" 83 "cannot read $local_config_file"
    }
  }

  if { [ord::db_has_rows] } {
    mpl::place_macros
  } else {
    utl::error "MPL" 89 "No rows found. Use initialize_floorplan to add rows."
  }
}
