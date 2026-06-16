# LEF Grammar Railroad Diagrams

The LEF (Library Exchange Format) grammar defines the syntax for the
library description files read and written by OpenROAD's ODB module.  The grammar is
implemented as a Bison parser in
[`src/odb/src/lef/lef/lef.y`](../src/lef/lef/lef.y).

Railroad diagrams (also called syntax diagrams) give an at-a-glance visual
summary of each grammar rule.  The SVGs below are generated directly from
`lef.y` by [`generate_railroad_diagrams.py`](generate_railroad_diagrams.py)
and are regenerated automatically via CI whenever `lef.y` changes.

## Contents

- [File structure](#file-structure)
- [Units](#units)
- [Layers](#layers)
- [Vias](#vias)
- [Via Rules](#via-rules)
- [Spacing](#spacing)
- [Sites](#sites)
- [Macros](#macros)
- [Macro Pins](#macro-pins)
- [Non-default Rules](#non-default-rules)
- [Properties](#properties)
- [IRDrop](#irdrop)
- [Noise & Correction Tables](#noise-correction-tables)
- [Timing](#timing)
- [Arrays (Floorplan)](#arrays-floorplan)
- [Antenna Rules](#antenna-rules)
- [Dielectric & Minfeature](#dielectric-minfeature)
- [Primitives](#primitives)

## File structure

### `lef_file`

![lef_file](./images/lef/lef_file.svg)

### `rule`

![rule](./images/lef/rule.svg)

### `version`

![version](./images/lef/version.svg)

### `end_library`

![end_library](./images/lef/end_library.svg)

### `busbitchars`

![busbitchars](./images/lef/busbitchars.svg)

### `dividerchar`

![dividerchar](./images/lef/dividerchar.svg)

### `case_sensitivity`

![case_sensitivity](./images/lef/case_sensitivity.svg)

### `wireextension`

![wireextension](./images/lef/wireextension.svg)

### `fixedmask`

![fixedmask](./images/lef/fixedmask.svg)

### `manufacturing`

![manufacturing](./images/lef/manufacturing.svg)

### `useminspacing`

![useminspacing](./images/lef/useminspacing.svg)

### `clearancemeasure`

![clearancemeasure](./images/lef/clearancemeasure.svg)

### `clearance_type`

![clearance_type](./images/lef/clearance_type.svg)

### `spacing_type`

![spacing_type](./images/lef/spacing_type.svg)

### `spacing_value`

![spacing_value](./images/lef/spacing_value.svg)

### `maxstack_via`

![maxstack_via](./images/lef/maxstack_via.svg)

### `create_file_statement`

![create_file_statement](./images/lef/create_file_statement.svg)

### `extension`

![extension](./images/lef/extension.svg)

### `def_statement`

![def_statement](./images/lef/def_statement.svg)

### `msg_statement`

![msg_statement](./images/lef/msg_statement.svg)

## Units

### `units_section`

![units_section](./images/lef/units_section.svg)

### `start_units`

![start_units](./images/lef/start_units.svg)

### `units_rule`

![units_rule](./images/lef/units_rule.svg)

## Layers

### `layer_rule`

![layer_rule](./images/lef/layer_rule.svg)

### `start_layer`

![start_layer](./images/lef/start_layer.svg)

### `end_layer`

![end_layer](./images/lef/end_layer.svg)

### `layer_option`

![layer_option](./images/lef/layer_option.svg)

### `layer_type`

![layer_type](./images/lef/layer_type.svg)

### `layer_direction`

![layer_direction](./images/lef/layer_direction.svg)

### `layer_name`

![layer_name](./images/lef/layer_name.svg)

### `layer_prop`

![layer_prop](./images/lef/layer_prop.svg)

### `layer_spacing`

![layer_spacing](./images/lef/layer_spacing.svg)

### `layer_spacing_cut_routing`

![layer_spacing_cut_routing](./images/lef/layer_spacing_cut_routing.svg)

### `layer_spacing_opt`

![layer_spacing_opt](./images/lef/layer_spacing_opt.svg)

### `layer_spacingtable_opt`

![layer_spacingtable_opt](./images/lef/layer_spacingtable_opt.svg)

### `layer_enclosure_type_opt`

![layer_enclosure_type_opt](./images/lef/layer_enclosure_type_opt.svg)

### `layer_enclosure_width_opt`

![layer_enclosure_width_opt](./images/lef/layer_enclosure_width_opt.svg)

### `layer_enclosure_width_except_opt`

![layer_enclosure_width_except_opt](./images/lef/layer_enclosure_width_except_opt.svg)

### `layer_preferenclosure_width_opt`

![layer_preferenclosure_width_opt](./images/lef/layer_preferenclosure_width_opt.svg)

### `layer_minimumcut_within`

![layer_minimumcut_within](./images/lef/layer_minimumcut_within.svg)

### `layer_minimumcut_from`

![layer_minimumcut_from](./images/lef/layer_minimumcut_from.svg)

### `layer_minimumcut_length`

![layer_minimumcut_length](./images/lef/layer_minimumcut_length.svg)

### `layer_minstep_option`

![layer_minstep_option](./images/lef/layer_minstep_option.svg)

### `layer_minstep_type`

![layer_minstep_type](./images/lef/layer_minstep_type.svg)

### `layer_minen_width`

![layer_minen_width](./images/lef/layer_minen_width.svg)

### `layer_oxide`

![layer_oxide](./images/lef/layer_oxide.svg)

### `layer_exceptpgnet`

![layer_exceptpgnet](./images/lef/layer_exceptpgnet.svg)

### `layer_arraySpacing_long`

![layer_arraySpacing_long](./images/lef/layer_arraySpacing_long.svg)

### `layer_arraySpacing_width`

![layer_arraySpacing_width](./images/lef/layer_arraySpacing_width.svg)

### `layer_arraySpacing_arraycut`

![layer_arraySpacing_arraycut](./images/lef/layer_arraySpacing_arraycut.svg)

### `layer_sp_parallel_width`

![layer_sp_parallel_width](./images/lef/layer_sp_parallel_width.svg)

### `layer_sp_TwoWidth`

![layer_sp_TwoWidth](./images/lef/layer_sp_TwoWidth.svg)

### `layer_sp_TwoWidthsPRL`

![layer_sp_TwoWidthsPRL](./images/lef/layer_sp_TwoWidthsPRL.svg)

### `layer_sp_influence_width`

![layer_sp_influence_width](./images/lef/layer_sp_influence_width.svg)

### `layer_antenna_duo`

![layer_antenna_duo](./images/lef/layer_antenna_duo.svg)

### `layer_antenna_pwl`

![layer_antenna_pwl](./images/lef/layer_antenna_pwl.svg)

### `layer_diffusion_ratio`

![layer_diffusion_ratio](./images/lef/layer_diffusion_ratio.svg)

### `layer_diffusion_ratios`

![layer_diffusion_ratios](./images/lef/layer_diffusion_ratios.svg)

### `layer_table_type`

![layer_table_type](./images/lef/layer_table_type.svg)

### `layer_frequency`

![layer_frequency](./images/lef/layer_frequency.svg)

### `ac_layer_table_opt`

![ac_layer_table_opt](./images/lef/ac_layer_table_opt.svg)

### `dc_layer_table`

![dc_layer_table](./images/lef/dc_layer_table.svg)

### `sp_options`

![sp_options](./images/lef/sp_options.svg)

### `res_point`

![res_point](./images/lef/res_point.svg)

### `cap_point`

![cap_point](./images/lef/cap_point.svg)

### `current_density_pwl`

![current_density_pwl](./images/lef/current_density_pwl.svg)

### `spacing_cut_layer_opt`

![spacing_cut_layer_opt](./images/lef/spacing_cut_layer_opt.svg)

## Vias

### `via`

![via](./images/lef/via.svg)

### `via_keyword`

![via_keyword](./images/lef/via_keyword.svg)

### `start_via`

![start_via](./images/lef/start_via.svg)

### `end_via`

![end_via](./images/lef/end_via.svg)

### `via_option`

![via_option](./images/lef/via_option.svg)

### `via_other_options`

![via_other_options](./images/lef/via_other_options.svg)

### `via_other_option`

![via_other_option](./images/lef/via_other_option.svg)

### `via_viarule`

![via_viarule](./images/lef/via_viarule.svg)

### `via_viarule_option`

![via_viarule_option](./images/lef/via_viarule_option.svg)

### `via_foreign`

![via_foreign](./images/lef/via_foreign.svg)

### `start_foreign`

![start_foreign](./images/lef/start_foreign.svg)

### `via_layer_rule`

![via_layer_rule](./images/lef/via_layer_rule.svg)

### `via_layer`

![via_layer](./images/lef/via_layer.svg)

### `via_geometry`

![via_geometry](./images/lef/via_geometry.svg)

### `via_placement`

![via_placement](./images/lef/via_placement.svg)

### `via_name`

![via_name](./images/lef/via_name.svg)

### `via_name_value_pair`

![via_name_value_pair](./images/lef/via_name_value_pair.svg)

## Via Rules

### `viarule`

![viarule](./images/lef/viarule.svg)

### `viarule_generate`

![viarule_generate](./images/lef/viarule_generate.svg)

### `viarule_keyword`

![viarule_keyword](./images/lef/viarule_keyword.svg)

### `viarule_generate_default`

![viarule_generate_default](./images/lef/viarule_generate_default.svg)

### `viarule_layer_list`

![viarule_layer_list](./images/lef/viarule_layer_list.svg)

### `viarule_layer`

![viarule_layer](./images/lef/viarule_layer.svg)

### `viarule_layer_name`

![viarule_layer_name](./images/lef/viarule_layer_name.svg)

### `viarule_layer_option`

![viarule_layer_option](./images/lef/viarule_layer_option.svg)

### `viarule_prop`

![viarule_prop](./images/lef/viarule_prop.svg)

### `opt_viarule_props`

![opt_viarule_props](./images/lef/opt_viarule_props.svg)

## Spacing

### `spacing_rule`

![spacing_rule](./images/lef/spacing_rule.svg)

### `start_spacing`

![start_spacing](./images/lef/start_spacing.svg)

### `end_spacing`

![end_spacing](./images/lef/end_spacing.svg)

### `spacing`

![spacing](./images/lef/spacing.svg)

### `samenet_keyword`

![samenet_keyword](./images/lef/samenet_keyword.svg)

### `opt_samenetPGonly`

![opt_samenetPGonly](./images/lef/opt_samenetPGonly.svg)

### `opt_adjacentcuts_exceptsame`

![opt_adjacentcuts_exceptsame](./images/lef/opt_adjacentcuts_exceptsame.svg)

### `opt_endofline`

![opt_endofline](./images/lef/opt_endofline.svg)

### `opt_endofline_twoedges`

![opt_endofline_twoedges](./images/lef/opt_endofline_twoedges.svg)

### `opt_range_second`

![opt_range_second](./images/lef/opt_range_second.svg)

### `opt_def_value`

![opt_def_value](./images/lef/opt_def_value.svg)

### `opt_def_range`

![opt_def_range](./images/lef/opt_def_range.svg)

### `opt_def_dvalue`

![opt_def_dvalue](./images/lef/opt_def_dvalue.svg)

## Sites

### `site`

![site](./images/lef/site.svg)

### `start_site`

![start_site](./images/lef/start_site.svg)

### `end_site`

![end_site](./images/lef/end_site.svg)

### `site_option`

![site_option](./images/lef/site_option.svg)

### `site_class`

![site_class](./images/lef/site_class.svg)

### `site_symmetry`

![site_symmetry](./images/lef/site_symmetry.svg)

### `site_symmetry_statement`

![site_symmetry_statement](./images/lef/site_symmetry_statement.svg)

### `site_word`

![site_word](./images/lef/site_word.svg)

### `site_rowpattern`

![site_rowpattern](./images/lef/site_rowpattern.svg)

### `site_rowpattern_statement`

![site_rowpattern_statement](./images/lef/site_rowpattern_statement.svg)

### `sitePattern`

![sitePattern](./images/lef/sitePattern.svg)

### `core_type`

![core_type](./images/lef/core_type.svg)

### `pad_type`

![pad_type](./images/lef/pad_type.svg)

### `endcap_type`

![endcap_type](./images/lef/endcap_type.svg)

## Macros

### `macro`

![macro](./images/lef/macro.svg)

### `start_macro`

![start_macro](./images/lef/start_macro.svg)

### `end_macro`

![end_macro](./images/lef/end_macro.svg)

### `macro_option`

![macro_option](./images/lef/macro_option.svg)

### `macro_class`

![macro_class](./images/lef/macro_class.svg)

### `macro_source`

![macro_source](./images/lef/macro_source.svg)

### `macro_symmetry`

![macro_symmetry](./images/lef/macro_symmetry.svg)

### `macro_symmetry_statement`

![macro_symmetry_statement](./images/lef/macro_symmetry_statement.svg)

### `macro_size`

![macro_size](./images/lef/macro_size.svg)

### `macro_origin`

![macro_origin](./images/lef/macro_origin.svg)

### `macro_foreign`

![macro_foreign](./images/lef/macro_foreign.svg)

### `macro_eeq`

![macro_eeq](./images/lef/macro_eeq.svg)

### `macro_leq`

![macro_leq](./images/lef/macro_leq.svg)

### `macro_generator`

![macro_generator](./images/lef/macro_generator.svg)

### `macro_generate`

![macro_generate](./images/lef/macro_generate.svg)

### `macro_clocktype`

![macro_clocktype](./images/lef/macro_clocktype.svg)

### `macro_power`

![macro_power](./images/lef/macro_power.svg)

### `macro_fixedMask`

![macro_fixedMask](./images/lef/macro_fixedMask.svg)

### `macro_density`

![macro_density](./images/lef/macro_density.svg)

### `density_layer`

![density_layer](./images/lef/density_layer.svg)

### `density_layer_rect`

![density_layer_rect](./images/lef/density_layer_rect.svg)

## Macro Pins

### `macro_pin`

![macro_pin](./images/lef/macro_pin.svg)

### `start_macro_pin`

![start_macro_pin](./images/lef/start_macro_pin.svg)

### `end_macro_pin`

![end_macro_pin](./images/lef/end_macro_pin.svg)

### `macro_pin_option`

![macro_pin_option](./images/lef/macro_pin_option.svg)

### `macro_pin_use`

![macro_pin_use](./images/lef/macro_pin_use.svg)

### `macro_scan_use`

![macro_scan_use](./images/lef/macro_scan_use.svg)

### `macro_port_class_option`

![macro_port_class_option](./images/lef/macro_port_class_option.svg)

### `macro_site`

![macro_site](./images/lef/macro_site.svg)

### `macro_site_word`

![macro_site_word](./images/lef/macro_site_word.svg)

### `start_macro_port`

![start_macro_port](./images/lef/start_macro_port.svg)

### `macro_obs`

![macro_obs](./images/lef/macro_obs.svg)

### `start_macro_obs`

![start_macro_obs](./images/lef/start_macro_obs.svg)

### `pin_shape`

![pin_shape](./images/lef/pin_shape.svg)

### `pin_layer_oxide`

![pin_layer_oxide](./images/lef/pin_layer_oxide.svg)

### `pin_name_value_pair`

![pin_name_value_pair](./images/lef/pin_name_value_pair.svg)

### `geometries`

![geometries](./images/lef/geometries.svg)

### `geometry`

![geometry](./images/lef/geometry.svg)

### `maskColor`

![maskColor](./images/lef/maskColor.svg)

## Non-default Rules

### `nondefault_rule`

![nondefault_rule](./images/lef/nondefault_rule.svg)

### `end_nd_rule`

![end_nd_rule](./images/lef/end_nd_rule.svg)

### `nd_hardspacing`

![nd_hardspacing](./images/lef/nd_hardspacing.svg)

### `nd_rule`

![nd_rule](./images/lef/nd_rule.svg)

### `nd_layer`

![nd_layer](./images/lef/nd_layer.svg)

### `nd_layer_stmt`

![nd_layer_stmt](./images/lef/nd_layer_stmt.svg)

### `nd_prop`

![nd_prop](./images/lef/nd_prop.svg)

### `usevia`

![usevia](./images/lef/usevia.svg)

### `useviarule`

![useviarule](./images/lef/useviarule.svg)

### `mincuts`

![mincuts](./images/lef/mincuts.svg)

## Properties

### `prop_def_section`

![prop_def_section](./images/lef/prop_def_section.svg)

### `prop_define`

![prop_define](./images/lef/prop_define.svg)

### `prop_stmt`

![prop_stmt](./images/lef/prop_stmt.svg)

### `relop`

![relop](./images/lef/relop.svg)

## IRDrop

### `irdrop`

![irdrop](./images/lef/irdrop.svg)

### `start_irdrop`

![start_irdrop](./images/lef/start_irdrop.svg)

### `end_irdrop`

![end_irdrop](./images/lef/end_irdrop.svg)

### `ir_table`

![ir_table](./images/lef/ir_table.svg)

### `ir_tablename`

![ir_tablename](./images/lef/ir_tablename.svg)

### `ir_table_value`

![ir_table_value](./images/lef/ir_table_value.svg)

## Noise & Correction Tables

### `noisetable`

![noisetable](./images/lef/noisetable.svg)

### `end_noisetable`

![end_noisetable](./images/lef/end_noisetable.svg)

### `noise_table_entry`

![noise_table_entry](./images/lef/noise_table_entry.svg)

### `correctiontable`

![correctiontable](./images/lef/correctiontable.svg)

### `end_correctiontable`

![end_correctiontable](./images/lef/end_correctiontable.svg)

### `correction_table_item`

![correction_table_item](./images/lef/correction_table_item.svg)

### `corr_victim`

![corr_victim](./images/lef/corr_victim.svg)

### `victim`

![victim](./images/lef/victim.svg)

### `universalnoisemargin`

![universalnoisemargin](./images/lef/universalnoisemargin.svg)

### `edgeratethreshold1`

![edgeratethreshold1](./images/lef/edgeratethreshold1.svg)

### `edgeratescalefactor`

![edgeratescalefactor](./images/lef/edgeratescalefactor.svg)

### `edgeratethreshold2`

![edgeratethreshold2](./images/lef/edgeratethreshold2.svg)

## Timing

### `timing`

![timing](./images/lef/timing.svg)

### `start_timing`

![start_timing](./images/lef/start_timing.svg)

### `end_timing`

![end_timing](./images/lef/end_timing.svg)

### `timing_option`

![timing_option](./images/lef/timing_option.svg)

### `slew_spec`

![slew_spec](./images/lef/slew_spec.svg)

### `delay_or_transition`

![delay_or_transition](./images/lef/delay_or_transition.svg)

### `risefall`

![risefall](./images/lef/risefall.svg)

### `unateness`

![unateness](./images/lef/unateness.svg)

### `electrical_direction`

![electrical_direction](./images/lef/electrical_direction.svg)

### `one_pin_trigger`

![one_pin_trigger](./images/lef/one_pin_trigger.svg)

### `two_pin_trigger`

![two_pin_trigger](./images/lef/two_pin_trigger.svg)

### `from_pin_trigger`

![from_pin_trigger](./images/lef/from_pin_trigger.svg)

### `to_pin_trigger`

![to_pin_trigger](./images/lef/to_pin_trigger.svg)

### `table_entry`

![table_entry](./images/lef/table_entry.svg)

### `output_list`

![output_list](./images/lef/output_list.svg)

### `output_resistance_entry`

![output_resistance_entry](./images/lef/output_resistance_entry.svg)

### `one_cap`

![one_cap](./images/lef/one_cap.svg)

### `then`

![then](./images/lef/then.svg)

### `else`

![else](./images/lef/else.svg)

### `b_expr`

![b_expr](./images/lef/b_expr.svg)

### `s_expr`

![s_expr](./images/lef/s_expr.svg)

### `expression`

![expression](./images/lef/expression.svg)

### `dtrm`

![dtrm](./images/lef/dtrm.svg)

## Arrays (Floorplan)

### `array`

![array](./images/lef/array.svg)

### `start_array`

![start_array](./images/lef/start_array.svg)

### `end_array`

![end_array](./images/lef/end_array.svg)

### `array_rule`

![array_rule](./images/lef/array_rule.svg)

### `gcellPattern`

![gcellPattern](./images/lef/gcellPattern.svg)

### `trackPattern`

![trackPattern](./images/lef/trackPattern.svg)

### `sitePattern`

![sitePattern](./images/lef/sitePattern.svg)

### `stepPattern`

![stepPattern](./images/lef/stepPattern.svg)

### `floorplan_start`

![floorplan_start](./images/lef/floorplan_start.svg)

### `floorplan_element`

![floorplan_element](./images/lef/floorplan_element.svg)

## Antenna Rules

### `input_antenna`

![input_antenna](./images/lef/input_antenna.svg)

### `output_antenna`

![output_antenna](./images/lef/output_antenna.svg)

### `inout_antenna`

![inout_antenna](./images/lef/inout_antenna.svg)

### `antenna_input`

![antenna_input](./images/lef/antenna_input.svg)

### `antenna_inout`

![antenna_inout](./images/lef/antenna_inout.svg)

### `antenna_output`

![antenna_output](./images/lef/antenna_output.svg)

## Dielectric & Minfeature

### `dielectric`

![dielectric](./images/lef/dielectric.svg)

### `minfeature`

![minfeature](./images/lef/minfeature.svg)

## Primitives

### `pt`

![pt](./images/lef/pt.svg)

### `firstPt`

![firstPt](./images/lef/firstPt.svg)

### `nextPt`

![nextPt](./images/lef/nextPt.svg)

### `otherPts`

![otherPts](./images/lef/otherPts.svg)

### `orientation`

![orientation](./images/lef/orientation.svg)

### `int_number`

![int_number](./images/lef/int_number.svg)

### `int_number_list`

![int_number_list](./images/lef/int_number_list.svg)

### `number_list`

![number_list](./images/lef/number_list.svg)

### `opt_layer_name`

![opt_layer_name](./images/lef/opt_layer_name.svg)

### `req_layer_name`

![req_layer_name](./images/lef/req_layer_name.svg)

## Regenerating the diagrams

After editing `lef.y`, re-run:

```shell
python3 src/odb/doc/generate_railroad_diagrams.py lef
```

Java 11 or later must be on `PATH`.  The WAR tools are vendored in
`src/odb/doc/tools/` and are not downloaded at runtime.
