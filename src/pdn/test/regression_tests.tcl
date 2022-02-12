# Record tests in /test
record_tests {
  reset
  report
  ripup
  convert
  
  min_width
  max_width
  min_spacing
  widthtable

  core_grid
  core_grid_with_rings
  core_grid_start_ground
  core_grid_start_ground_strap_power
  core_grid_with_rings_with_straps
  core_grid_dual_followpins
  core_grid_with_dual_rings
  core_grid_with_rings_connect
  core_grid_cut_pitch
  core_grid_snap
  
  core_grid_obstruction

  core_grid_auto_domain

  core_grid_extend_to_boundary
  core_grid_extend_to_boundary_no_pins
  core_grid_with_M7_pins

  core_grid_strap_count

  core_grid_no_trim

  macros
  macros_with_halo
  macros_cells
  macros_with_rings
  macros_narrow_channel
  macros_narrow_channel_large_spacing
  macros_narrow_channel_repair_overlap
  
  region_temp_sensor
  region_secondary_nets
  region_non_rect

  pads_black_parrot
  pads_black_parrot_offset
  pads_black_parrot_no_connect
  pads_black_parrot_limit_connect
  pads_black_parrot_flipchip
  
  asap7_vias
  asap7_vias_cutclass
  asap7_no_via_generate
  asap7_vias_arrayspacing
  asap7_vias_arrayspacing_notfirst
  asap7_vias_arrayspacing_partial
  asap7_vias_arrayspacing_3_layer
  }
