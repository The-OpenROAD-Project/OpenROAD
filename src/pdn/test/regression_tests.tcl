# Record tests in /test
record_tests {
  reset
  report
  ripup
  convert
  names

  min_width
  max_width
  min_spacing
  widthtable
  design_width
  offgrid

  core_grid
  core_grid_with_rings
  core_grid_start_power
  core_grid_start_power_strap_ground
  core_grid_with_rings_with_straps
  core_grid_dual_followpins
  core_grid_dual_followpins_error
  core_grid_with_dual_rings
  core_grid_with_rings_connect
  core_grid_cut_pitch
  core_grid_snap
  core_grid_via_snap
  core_grid_split_cuts
  core_grid_with_rings_with_straps_rings_over_core
  core_grid_with_routing_obstructions
  core_grid_adjacentcuts

  core_grid_obstruction

  core_grid_auto_domain
  core_grid_auto_domain_multiple_nets

  core_grid_extend_to_boundary
  core_grid_extend_to_boundary_no_pins
  core_grid_with_M7_pins

  core_grid_strap_count

  core_grid_no_trim
  
  core_grid_offset_strap
  core_grid_with_rings_with_limit_straps

  core_grid_failed_via_report

  macros
  macros_with_halo
  macros_cells
  macros_cells_orient
  macros_with_rings
  macros_narrow_channel
  macros_narrow_channel_large_spacing
  macros_narrow_channel_repair_overlap
  macros_add_twice
  macros_cells_extend_boundary
  macros_cells_no_grid
  macros_narrow_channel_jog
  macros_different_nets
  macros_grid_through
  macros_grid_through_without_middle
  macros_cells_dont_touch
  macros_cells_overlapping_ports
  macros_cells_not_fixed

  region_temp_sensor
  region_secondary_nets
  region_non_rect

  pads_black_parrot
  pads_black_parrot_offset
  pads_black_parrot_no_connect
  pads_black_parrot_limit_connect
  pads_black_parrot_flipchip
  pads_black_parrot_flipchip_connect_bumps
  pads_black_parrot_flipchip_connect_overpads
  pads_black_parrot_max_width

  asap7_vias
  asap7_vias_cutclass
  asap7_no_via_generate
  asap7_vias_arrayspacing
  asap7_vias_arrayspacing_notfirst
  asap7_vias_arrayspacing_partial
  asap7_vias_arrayspacing_3_layer
  asap7_vias_max_rows_columns
  asap7_vias_dont_use
  asap7_taper
  asap7_offcenter_via
  asap7_no_via_generate_v1_snapped
  asap7_failed_macro_grid
  asap7_vias_fixed_vias

  existing

  power_switch
  power_switch_star
  power_switch_daisy
  power_switch_regions
  power_switch_cut_rows
  
  repair_vias

  sroute_test
}
