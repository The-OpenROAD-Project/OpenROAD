# DEF Grammar Railroad Diagrams

The DEF (Design Exchange Format) grammar defines the syntax for the
design description files read and written by OpenROAD's ODB module.  The grammar is
implemented as a Bison parser in
[`src/odb/src/def/def/def.y`](../src/def/def/def.y).

Railroad diagrams (also called syntax diagrams) give an at-a-glance visual
summary of each grammar rule.  The SVGs below are generated directly from
`def.y` by [`generate_railroad_diagrams.py`](generate_railroad_diagrams.py)
and are regenerated automatically via CI whenever `def.y` changes.

## Contents

- [File structure](#file-structure)
- [Design Attributes](#design-attributes)
- [Properties](#properties)
- [Components](#components)
- [Nets](#nets)
- [Net Routing](#net-routing)
- [Net Subnets](#net-subnets)
- [Special Nets](#special-nets)
- [Pins](#pins)
- [Virtual Pins](#virtual-pins)
- [Pin Properties](#pin-properties)
- [Blockages](#blockages)
- [Regions](#regions)
- [Groups](#groups)
- [Via Definitions](#via-definitions)
- [Non-default Rules](#non-default-rules)
- [Fills](#fills)
- [Slots](#slots)
- [Styles](#styles)
- [IO Timing](#io-timing)
- [Scan Chains](#scan-chains)
- [Timing Disables](#timing-disables)
- [Partitions](#partitions)
- [Constraints](#constraints)
- [Assertions](#assertions)
- [Floorplan Constraints](#floorplan-constraints)
- [Extension](#extension)
- [Primitives](#primitives)

## File structure

### `def_file`

![def_file](./images/def/def_file.svg)

### `rule`

![rule](./images/def/rule.svg)

### `version_stmt`

![version_stmt](./images/def/version_stmt.svg)

### `case_sens_stmt`

![case_sens_stmt](./images/def/case_sens_stmt.svg)

### `end_design`

![end_design](./images/def/end_design.svg)

### `design_name`

![design_name](./images/def/design_name.svg)

### `tech_name`

![tech_name](./images/def/tech_name.svg)

### `history`

![history](./images/def/history.svg)

### `bus_bit_chars`

![bus_bit_chars](./images/def/bus_bit_chars.svg)

### `divider_char`

![divider_char](./images/def/divider_char.svg)

### `array_name`

![array_name](./images/def/array_name.svg)

### `floorplan_name`

![floorplan_name](./images/def/floorplan_name.svg)

## Design Attributes

### `design_section`

![design_section](./images/def/design_section.svg)

### `die_area`

![die_area](./images/def/die_area.svg)

### `units`

![units](./images/def/units.svg)

### `gcellgrid`

![gcellgrid](./images/def/gcellgrid.svg)

### `tracks_rule`

![tracks_rule](./images/def/tracks_rule.svg)

### `track_start`

![track_start](./images/def/track_start.svg)

### `track_opts`

![track_opts](./images/def/track_opts.svg)

### `track_type`

![track_type](./images/def/track_type.svg)

### `track_layer`

![track_layer](./images/def/track_layer.svg)

### `track_layer_statement`

![track_layer_statement](./images/def/track_layer_statement.svg)

### `track_mask_statement`

![track_mask_statement](./images/def/track_mask_statement.svg)

### `row_rule`

![row_rule](./images/def/row_rule.svg)

### `row_option`

![row_option](./images/def/row_option.svg)

### `row_step_option`

![row_step_option](./images/def/row_step_option.svg)

### `row_do_option`

![row_do_option](./images/def/row_do_option.svg)

### `row_prop`

![row_prop](./images/def/row_prop.svg)

### `row_or_comp`

![row_or_comp](./images/def/row_or_comp.svg)

## Properties

### `prop_def_section`

![prop_def_section](./images/def/prop_def_section.svg)

### `property_def`

![property_def](./images/def/property_def.svg)

### `property_type_and_val`

![property_type_and_val](./images/def/property_type_and_val.svg)

## Components

### `comps_section`

![comps_section](./images/def/comps_section.svg)

### `comps_maskShift_section`

![comps_maskShift_section](./images/def/comps_maskShift_section.svg)

### `start_comps`

![start_comps](./images/def/start_comps.svg)

### `end_comps`

![end_comps](./images/def/end_comps.svg)

### `comp`

![comp](./images/def/comp.svg)

### `comp_start`

![comp_start](./images/def/comp_start.svg)

### `comp_id_and_name`

![comp_id_and_name](./images/def/comp_id_and_name.svg)

### `comp_name`

![comp_name](./images/def/comp_name.svg)

### `comp_option`

![comp_option](./images/def/comp_option.svg)

### `comp_type`

![comp_type](./images/def/comp_type.svg)

### `comp_source`

![comp_source](./images/def/comp_source.svg)

### `comp_eeq`

![comp_eeq](./images/def/comp_eeq.svg)

### `comp_foreign`

![comp_foreign](./images/def/comp_foreign.svg)

### `comp_generate`

![comp_generate](./images/def/comp_generate.svg)

### `comp_halo`

![comp_halo](./images/def/comp_halo.svg)

### `comp_routehalo`

![comp_routehalo](./images/def/comp_routehalo.svg)

### `comp_region`

![comp_region](./images/def/comp_region.svg)

### `comp_region_start`

![comp_region_start](./images/def/comp_region_start.svg)

### `comp_extension_stmt`

![comp_extension_stmt](./images/def/comp_extension_stmt.svg)

### `comp_prop`

![comp_prop](./images/def/comp_prop.svg)

### `comp_property`

![comp_property](./images/def/comp_property.svg)

### `comp_blockage_rule`

![comp_blockage_rule](./images/def/comp_blockage_rule.svg)

### `placement_comp_rule`

![placement_comp_rule](./images/def/placement_comp_rule.svg)

### `placement_status`

![placement_status](./images/def/placement_status.svg)

### `halo_soft`

![halo_soft](./images/def/halo_soft.svg)

## Nets

### `nets_section`

![nets_section](./images/def/nets_section.svg)

### `start_nets`

![start_nets](./images/def/start_nets.svg)

### `end_nets`

![end_nets](./images/def/end_nets.svg)

### `one_net`

![one_net](./images/def/one_net.svg)

### `net_and_connections`

![net_and_connections](./images/def/net_and_connections.svg)

### `net_start`

![net_start](./images/def/net_start.svg)

### `net_name`

![net_name](./images/def/net_name.svg)

### `net_connection`

![net_connection](./images/def/net_connection.svg)

### `net_option`

![net_option](./images/def/net_option.svg)

### `net_type`

![net_type](./images/def/net_type.svg)

### `net_prop`

![net_prop](./images/def/net_prop.svg)

### `netsource_type`

![netsource_type](./images/def/netsource_type.svg)

### `conn_opt`

![conn_opt](./images/def/conn_opt.svg)

### `use_type`

![use_type](./images/def/use_type.svg)

### `source_type`

![source_type](./images/def/source_type.svg)

## Net Routing

### `paths`

![paths](./images/def/paths.svg)

### `path`

![path](./images/def/path.svg)

### `new_path`

![new_path](./images/def/new_path.svg)

### `path_item`

![path_item](./images/def/path_item.svg)

### `path_item_list`

![path_item_list](./images/def/path_item_list.svg)

### `path_pt`

![path_pt](./images/def/path_pt.svg)

### `opt_taper`

![opt_taper](./images/def/opt_taper.svg)

### `opt_taper_style`

![opt_taper_style](./images/def/opt_taper_style.svg)

### `opt_shape_style`

![opt_shape_style](./images/def/opt_shape_style.svg)

### `opt_style`

![opt_style](./images/def/opt_style.svg)

### `opt_pattern`

![opt_pattern](./images/def/opt_pattern.svg)

### `weight`

![weight](./images/def/weight.svg)

### `width`

![width](./images/def/width.svg)

### `shield_layer`

![shield_layer](./images/def/shield_layer.svg)

## Net Subnets

### `subnet_option`

![subnet_option](./images/def/subnet_option.svg)

### `subnet_type`

![subnet_type](./images/def/subnet_type.svg)

### `subnet_opt_syn`

![subnet_opt_syn](./images/def/subnet_opt_syn.svg)

### `opt_common_pins`

![opt_common_pins](./images/def/opt_common_pins.svg)

### `floating_pins`

![floating_pins](./images/def/floating_pins.svg)

### `ordered_pins`

![ordered_pins](./images/def/ordered_pins.svg)

### `one_floating_inst`

![one_floating_inst](./images/def/one_floating_inst.svg)

### `one_ordered_inst`

![one_ordered_inst](./images/def/one_ordered_inst.svg)

### `opt_pin`

![opt_pin](./images/def/opt_pin.svg)

## Special Nets

### `snets_section`

![snets_section](./images/def/snets_section.svg)

### `start_snets`

![start_snets](./images/def/start_snets.svg)

### `end_snets`

![end_snets](./images/def/end_snets.svg)

### `snet_rule`

![snet_rule](./images/def/snet_rule.svg)

### `snet_option`

![snet_option](./images/def/snet_option.svg)

### `snet_other_option`

![snet_other_option](./images/def/snet_other_option.svg)

### `snet_spacing`

![snet_spacing](./images/def/snet_spacing.svg)

### `snet_voltage`

![snet_voltage](./images/def/snet_voltage.svg)

### `snet_width`

![snet_width](./images/def/snet_width.svg)

### `snet_prop`

![snet_prop](./images/def/snet_prop.svg)

### `spaths`

![spaths](./images/def/spaths.svg)

### `spath`

![spath](./images/def/spath.svg)

### `snew_path`

![snew_path](./images/def/snew_path.svg)

### `geom_fill`

![geom_fill](./images/def/geom_fill.svg)

### `geom_slot`

![geom_slot](./images/def/geom_slot.svg)

## Pins

### `pin_rule`

![pin_rule](./images/def/pin_rule.svg)

### `start_pins`

![start_pins](./images/def/start_pins.svg)

### `end_pins`

![end_pins](./images/def/end_pins.svg)

### `pin_cap_rule`

![pin_cap_rule](./images/def/pin_cap_rule.svg)

### `pin_cap`

![pin_cap](./images/def/pin_cap.svg)

### `pin`

![pin](./images/def/pin.svg)

### `pin_option`

![pin_option](./images/def/pin_option.svg)

### `pin_layer_opt`

![pin_layer_opt](./images/def/pin_layer_opt.svg)

### `pin_layer_mask_opt`

![pin_layer_mask_opt](./images/def/pin_layer_mask_opt.svg)

### `pin_layer_spacing_opt`

![pin_layer_spacing_opt](./images/def/pin_layer_spacing_opt.svg)

### `pin_via_mask_opt`

![pin_via_mask_opt](./images/def/pin_via_mask_opt.svg)

### `pin_poly_mask_opt`

![pin_poly_mask_opt](./images/def/pin_poly_mask_opt.svg)

### `pin_poly_spacing_opt`

![pin_poly_spacing_opt](./images/def/pin_poly_spacing_opt.svg)

### `pin_oxide`

![pin_oxide](./images/def/pin_oxide.svg)

## Virtual Pins

### `vpin_stmt`

![vpin_stmt](./images/def/vpin_stmt.svg)

### `vpin_begin`

![vpin_begin](./images/def/vpin_begin.svg)

### `vpin_layer_opt`

![vpin_layer_opt](./images/def/vpin_layer_opt.svg)

### `vpin_options`

![vpin_options](./images/def/vpin_options.svg)

### `vpin_status`

![vpin_status](./images/def/vpin_status.svg)

## Pin Properties

### `pin_props_section`

![pin_props_section](./images/def/pin_props_section.svg)

### `begin_pin_props`

![begin_pin_props](./images/def/begin_pin_props.svg)

### `end_pin_props`

![end_pin_props](./images/def/end_pin_props.svg)

### `pin_prop_terminal`

![pin_prop_terminal](./images/def/pin_prop_terminal.svg)

### `pin_prop`

![pin_prop](./images/def/pin_prop.svg)

### `pin_prop_name_value`

![pin_prop_name_value](./images/def/pin_prop_name_value.svg)

## Blockages

### `blockage_section`

![blockage_section](./images/def/blockage_section.svg)

### `blockage_start`

![blockage_start](./images/def/blockage_start.svg)

### `blockage_end`

![blockage_end](./images/def/blockage_end.svg)

### `blockage_def`

![blockage_def](./images/def/blockage_def.svg)

### `blockage_rule`

![blockage_rule](./images/def/blockage_rule.svg)

### `layer_blockage_rule`

![layer_blockage_rule](./images/def/layer_blockage_rule.svg)

### `mask_blockage_rule`

![mask_blockage_rule](./images/def/mask_blockage_rule.svg)

### `comp_blockage_rule`

![comp_blockage_rule](./images/def/comp_blockage_rule.svg)

### `rectPoly_blockage`

![rectPoly_blockage](./images/def/rectPoly_blockage.svg)

### `rect_statement`

![rect_statement](./images/def/rect_statement.svg)

### `rect_pts`

![rect_pts](./images/def/rect_pts.svg)

## Regions

### `regions_section`

![regions_section](./images/def/regions_section.svg)

### `regions_start`

![regions_start](./images/def/regions_start.svg)

### `regions_stmt`

![regions_stmt](./images/def/regions_stmt.svg)

### `region_option`

![region_option](./images/def/region_option.svg)

### `region_type`

![region_type](./images/def/region_type.svg)

### `region_prop`

![region_prop](./images/def/region_prop.svg)

## Groups

### `groups_section`

![groups_section](./images/def/groups_section.svg)

### `groups_start`

![groups_start](./images/def/groups_start.svg)

### `groups_end`

![groups_end](./images/def/groups_end.svg)

### `start_group`

![start_group](./images/def/start_group.svg)

### `group_rule`

![group_rule](./images/def/group_rule.svg)

### `group_member`

![group_member](./images/def/group_member.svg)

### `group_option`

![group_option](./images/def/group_option.svg)

### `group_region`

![group_region](./images/def/group_region.svg)

### `group_soft_option`

![group_soft_option](./images/def/group_soft_option.svg)

### `group_prop`

![group_prop](./images/def/group_prop.svg)

## Via Definitions

### `via_section`

![via_section](./images/def/via_section.svg)

### `start_def_cap`

![start_def_cap](./images/def/start_def_cap.svg)

### `end_def_cap`

![end_def_cap](./images/def/end_def_cap.svg)

### `via`

![via](./images/def/via.svg)

### `via_declaration`

![via_declaration](./images/def/via_declaration.svg)

### `via_end`

![via_end](./images/def/via_end.svg)

### `layer_stmt`

![layer_stmt](./images/def/layer_stmt.svg)

### `layer_viarule_opts`

![layer_viarule_opts](./images/def/layer_viarule_opts.svg)

## Non-default Rules

### `nondefaultrule_section`

![nondefaultrule_section](./images/def/nondefaultrule_section.svg)

### `nondefault_start`

![nondefault_start](./images/def/nondefault_start.svg)

### `nondefault_end`

![nondefault_end](./images/def/nondefault_end.svg)

### `nondefault_def`

![nondefault_def](./images/def/nondefault_def.svg)

### `nondefault_option`

![nondefault_option](./images/def/nondefault_option.svg)

### `nondefault_layer_option`

![nondefault_layer_option](./images/def/nondefault_layer_option.svg)

### `nondefault_prop`

![nondefault_prop](./images/def/nondefault_prop.svg)

### `nondefault_prop_opt`

![nondefault_prop_opt](./images/def/nondefault_prop_opt.svg)

## Fills

### `fill_section`

![fill_section](./images/def/fill_section.svg)

### `fill_start`

![fill_start](./images/def/fill_start.svg)

### `fill_end`

![fill_end](./images/def/fill_end.svg)

### `fill_rule`

![fill_rule](./images/def/fill_rule.svg)

### `fill_def`

![fill_def](./images/def/fill_def.svg)

### `fill_layer_opc`

![fill_layer_opc](./images/def/fill_layer_opc.svg)

### `fill_mask`

![fill_mask](./images/def/fill_mask.svg)

### `fill_viaMask`

![fill_viaMask](./images/def/fill_viaMask.svg)

### `fill_via_opc`

![fill_via_opc](./images/def/fill_via_opc.svg)

### `fill_via_pt`

![fill_via_pt](./images/def/fill_via_pt.svg)

## Slots

### `slot_section`

![slot_section](./images/def/slot_section.svg)

### `slot_start`

![slot_start](./images/def/slot_start.svg)

### `slot_end`

![slot_end](./images/def/slot_end.svg)

### `slot_rule`

![slot_rule](./images/def/slot_rule.svg)

### `slot_def`

![slot_def](./images/def/slot_def.svg)

## Styles

### `styles_section`

![styles_section](./images/def/styles_section.svg)

### `styles_start`

![styles_start](./images/def/styles_start.svg)

### `styles_end`

![styles_end](./images/def/styles_end.svg)

### `styles_rule`

![styles_rule](./images/def/styles_rule.svg)

## IO Timing

### `iotiming_section`

![iotiming_section](./images/def/iotiming_section.svg)

### `start_iotiming`

![start_iotiming](./images/def/start_iotiming.svg)

### `iotiming_end`

![iotiming_end](./images/def/iotiming_end.svg)

### `iotiming_rule`

![iotiming_rule](./images/def/iotiming_rule.svg)

### `iotiming_member`

![iotiming_member](./images/def/iotiming_member.svg)

### `iotiming_frompin`

![iotiming_frompin](./images/def/iotiming_frompin.svg)

### `iotiming_parallel`

![iotiming_parallel](./images/def/iotiming_parallel.svg)

### `iotiming_drivecell_opt`

![iotiming_drivecell_opt](./images/def/iotiming_drivecell_opt.svg)

## Scan Chains

### `scanchains_section`

![scanchains_section](./images/def/scanchains_section.svg)

### `scanchain_start`

![scanchain_start](./images/def/scanchain_start.svg)

### `scanchain_end`

![scanchain_end](./images/def/scanchain_end.svg)

### `scan_rule`

![scan_rule](./images/def/scan_rule.svg)

### `scan_member`

![scan_member](./images/def/scan_member.svg)

### `td_macro_option`

![td_macro_option](./images/def/td_macro_option.svg)

## Timing Disables

### `timingdisables_section`

![timingdisables_section](./images/def/timingdisables_section.svg)

### `timingdisables_start`

![timingdisables_start](./images/def/timingdisables_start.svg)

### `timingdisables_end`

![timingdisables_end](./images/def/timingdisables_end.svg)

### `timingdisables_rule`

![timingdisables_rule](./images/def/timingdisables_rule.svg)

### `turnoff`

![turnoff](./images/def/turnoff.svg)

### `turnoff_hold`

![turnoff_hold](./images/def/turnoff_hold.svg)

### `turnoff_setup`

![turnoff_setup](./images/def/turnoff_setup.svg)

### `wiredlogic_rule`

![wiredlogic_rule](./images/def/wiredlogic_rule.svg)

## Partitions

### `partitions_section`

![partitions_section](./images/def/partitions_section.svg)

### `partitions_start`

![partitions_start](./images/def/partitions_start.svg)

### `partitions_end`

![partitions_end](./images/def/partitions_end.svg)

### `start_partition`

![start_partition](./images/def/start_partition.svg)

### `partition_rule`

![partition_rule](./images/def/partition_rule.svg)

### `partition_member`

![partition_member](./images/def/partition_member.svg)

### `partition_maxbits`

![partition_maxbits](./images/def/partition_maxbits.svg)

### `min_or_max_member`

![min_or_max_member](./images/def/min_or_max_member.svg)

### `minmaxpins`

![minmaxpins](./images/def/minmaxpins.svg)

## Constraints

### `constraint_section`

![constraint_section](./images/def/constraint_section.svg)

### `constraints_start`

![constraints_start](./images/def/constraints_start.svg)

### `constraints_end`

![constraints_end](./images/def/constraints_end.svg)

### `constraint_rules`

![constraint_rules](./images/def/constraint_rules.svg)

### `constraint_rule`

![constraint_rule](./images/def/constraint_rule.svg)

### `constraint_type`

![constraint_type](./images/def/constraint_type.svg)

### `constrain_what`

![constrain_what](./images/def/constrain_what.svg)

### `operand`

![operand](./images/def/operand.svg)

### `operand_rule`

![operand_rule](./images/def/operand_rule.svg)

### `delay_spec`

![delay_spec](./images/def/delay_spec.svg)

### `risefall`

![risefall](./images/def/risefall.svg)

### `risefallminmax1`

![risefallminmax1](./images/def/risefallminmax1.svg)

### `risefallminmax2`

![risefallminmax2](./images/def/risefallminmax2.svg)

## Assertions

### `assertions_section`

![assertions_section](./images/def/assertions_section.svg)

### `assertions_start`

![assertions_start](./images/def/assertions_start.svg)

### `assertions_end`

![assertions_end](./images/def/assertions_end.svg)

## Floorplan Constraints

### `floorplan_contraints_section`

![floorplan_contraints_section](./images/def/floorplan_contraints_section.svg)

### `fp_start`

![fp_start](./images/def/fp_start.svg)

### `fp_stmt`

![fp_stmt](./images/def/fp_stmt.svg)

### `canplace`

![canplace](./images/def/canplace.svg)

### `cannotoccupy`

![cannotoccupy](./images/def/cannotoccupy.svg)

## Extension

### `extension_section`

![extension_section](./images/def/extension_section.svg)

### `extension_stmt`

![extension_stmt](./images/def/extension_stmt.svg)

## Primitives

### `pt`

![pt](./images/def/pt.svg)

### `firstPt`

![firstPt](./images/def/firstPt.svg)

### `nextPt`

![nextPt](./images/def/nextPt.svg)

### `otherPts`

![otherPts](./images/def/otherPts.svg)

### `orient`

![orient](./images/def/orient.svg)

### `orient_pt`

![orient_pt](./images/def/orient_pt.svg)

### `virtual_pt`

![virtual_pt](./images/def/virtual_pt.svg)

### `virtual_statement`

![virtual_statement](./images/def/virtual_statement.svg)

### `opt_plus`

![opt_plus](./images/def/opt_plus.svg)

### `opt_semi`

![opt_semi](./images/def/opt_semi.svg)

### `opt_paren`

![opt_paren](./images/def/opt_paren.svg)

### `opt_num_val`

![opt_num_val](./images/def/opt_num_val.svg)

### `opt_range`

![opt_range](./images/def/opt_range.svg)

### `opt_snet_range`

![opt_snet_range](./images/def/opt_snet_range.svg)

### `h_or_v`

![h_or_v](./images/def/h_or_v.svg)

### `same_mask`

![same_mask](./images/def/same_mask.svg)

### `mask`

![mask](./images/def/mask.svg)

### `maskLayer`

![maskLayer](./images/def/maskLayer.svg)

### `maskShift`

![maskShift](./images/def/maskShift.svg)

### `opt_mask_opc`

![opt_mask_opc](./images/def/opt_mask_opc.svg)

### `opt_mask_opc_l`

![opt_mask_opc_l](./images/def/opt_mask_opc_l.svg)

### `pattern_type`

![pattern_type](./images/def/pattern_type.svg)

### `shape_type`

![shape_type](./images/def/shape_type.svg)

## Regenerating the diagrams

After editing `def.y`, re-run:

```shell
python3 src/odb/doc/generate_railroad_diagrams.py def
```

Java 11 or later must be on `PATH`.  The WAR tools are vendored in
`src/odb/doc/tools/` and are not downloaded at runtime.
