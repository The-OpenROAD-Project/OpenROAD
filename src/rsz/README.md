# Gate Resizer

Gate Resizer commands are described below.  The `resizer` commands stop when
the design area is `-max_utilization util` percent of the core area. `util`
is between 0 and 100.  The `resizer` stops and reports an error if the max
utilization is exceeded.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Set Wire RC

The `set_wire_rc` command sets the resistance and capacitance used to estimate
delay of routing wires.  Separate values can be specified for clock and data
nets with the `-signal` and `-clock` flags. Without either `-signal` or
`-clock` the resistance and capacitance for clocks and data nets are set.

```
# Either run 
set_wire_rc -clock ... -signal ... -layer ...

# Or
set_wire_rc -resistance ... -capacitance ...
```

```tcl
set_wire_rc 
    [-clock] 
    [-signal]
    [-data]
    [-corner corner]
    [-layers layers_list]

or 
set_wire_rc
    [-h_resistance res]
    [-h_capacitance cap]
    [-v_resistance res]
    [-v_capacitance cap]

or
set_wire_rc 
    [-clock] 
    [-signal]
    [-data]
    [-corner corner]
    [-layer layer_name]
or 
set_wire_rc
    [-resistance res]
    [-capacitance cap]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-clock` | Enable setting of RC for clock nets. |
| `-signal` | Enable setting of RC for signal nets. | 
| `-layers` | Use the LEF technology resistance and area/edge capacitance values for the layers. The values for each layers will be used for wires with the prefered layer direction, if 2 or more layers have the same prefered direction the avarege value is used for wires with that direction. This is used for a default width wire on the layer. |
| `-layer` | Use the LEF technology resistance and area/edge capacitance values for the layer. This is used for a default width wire on the layer. |
| `-resistance` | Resistance per unit length, units are from the first Liberty file read. |
| `-capacitance` | Capacitance per unit length, units are from the first Liberty file read. |
| `-h_resistance` | Resistance per unit length for horizontal wires, units are from the first Liberty file read. |
| `-h_capacitance` | Capacitance per unit length for horizontal wires, units are from the first Liberty file read. |
| `-v_resistance` | Resistance per unit length for vertical wires, units are from the first Liberty file read. |
| `-v_capacitance` | Capacitance per unit length for vertical wires, units are from the first Liberty file read. |


### Set Layer RC

The `set_layer_rc` command can be used to set the resistance and capacitance
for a layer or via. This is useful if these values are missing from the LEF file,
or to override the values in the LEF.

```tcl
set_layer_rc 
    [-layer layer]
    [-via via_layer]
    [-resistance res]
    [-capacitance cap]
    [-corner corner]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-layer` | Set layer name to modify. Note that the layer must be a routing layer. |
| `-via` | Select via layer name. Note that via resistance is per cut/via, not area-based. |
| `-resistance` | Resistance per unit length, same convention as `set_wire_rc`. |
| `-capacitance` | Capacitance per unit length, same convention as `set_wire_rc`. |
| `-corner` | Process corner to use. |

### Estimate Parasitics

Estimate RC parasitics based on placed component pin locations. If there are
no component locations, then no parasitics are added. The resistance and capacitance
values are per distance unit of a routing wire. Use the `set_units` command to check
units or `set_cmd_units` to change units. The goal is to represent "average"
routing layer resistance and capacitance. If the set_wire_rc command is not
called before resizing, then the default_wireload model specified in the first
Liberty file read or with the SDC set_wire_load command is used to make parasitics.

After the `global_route` command has been called, the global routing topology
and layers can be used to estimate parasitics  with the `-global_routing`
flag.

The optional argument `-spef_file` can be used to write the estimated parasitics using 
Standard Parasitic Exchange Format.

```tcl
estimate_parasitics
    -placement|-global_routing
    [-spef_file spef_file]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-placement` or `-global_routing` | Either of these flags must be set. Parasitics are estimated based after placement stage versus after global routing stage. |
| `-spef_file` | Optional. File name to write SPEF files. If more than one corner is available for the design, the files will be written as filename_corner.spef. |

### Set Don't Use

The `set_dont_use` command removes library cells from consideration by
the `resizer` engine and the `CTS` engine. `lib_cells` is a list of cells returned by `get_lib_cells`
or a list of cell names (`wildcards` allowed). For example, `DLY*` says do
not use cells with names that begin with `DLY` in all libraries.

```tcl
set_dont_use lib_cells 
```

### Unset Don't Use

The `unset_dont_use` command reverses the `set_dont_use` command.

```tcl
unset_dont_use lib_cells
```

### Reset Don't Use

The `reset_dont_use` restores the default dont use list.

```tcl
reset_dont_use
```

### Report Don't Use

The `report_dont_use` reports all the cells that are marked as dont use.

```tcl
report_dont_use
```

### Set Don't Touch

The `set_dont_touch` command prevents the resizer commands from
modifying instances or nets.

```tcl
set_dont_touch instances_nets 
```

### Unset Don't Touch

The `unset_dont_touch` command reverse the `set_dont_touch` command.

```tcl
unset_dont_touch instances_nets
```

### Report Don't Touch

The `report_dont_touch` reports all the instances and nets that are marked as dont touch.

```tcl
report_dont_touch
```

### Buffer Ports

The `buffer_ports -inputs` command adds a buffer between the input and its
loads.  The `buffer_ports -outputs` adds a buffer between the port driver
and the output port. Inserting buffers on input and output ports makes
the block input capacitances and output drives independent of the block
internals.

```tcl
buffer_ports 
    [-inputs] 
    [-outputs] 
    [-max_utilization util]
    [-buffer_cell buf_cell]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-inputs`, `-outputs` | Insert a buffer between the input and load, output and load respectively. The default behavior is `-inputs` and `-outputs` set if neither is specified. |
| `-max_utilization` | Defines the percentage of core area used. |

### Remove Buffers

Use the `remove_buffers` command to remove buffers inserted by synthesis. This
step is recommended before using `repair_design` so that there is more flexibility
in buffering nets.  If buffer instances are specified, only specified buffer instances
will be removed regardless of dont-touch or fixed cell.  Direct input port to output port
feedthrough buffers will not be removed.
If no buffer instances are specified, all buffers will be removed except those that are associated with
dont-touch, fixed cell or direct input port to output port feedthrough buffering.

```tcl
remove_buffers
    [ instances ]
```

### Balance Row Usage

Command description pending.

```tcl
balance_row_usage
```

### Repair Design

The `repair_design` command inserts buffers on nets to repair max slew, max
capacitance and max fanout violations, and on long wires to reduce RC delay in
the wire. It also resizes gates to normalize slews.  Use `estimate_parasitics
-placement` before `repair_design` to estimate parasitics considered
during repair. Placement-based parasitics cannot accurately predict
routed parasitics, so a margin can be used to "over-repair" the design
to compensate. 

```tcl
repair_design 
    [-max_wire_length max_length]
    [-slew_margin slew_margin]
    [-cap_margin cap_margin]
    [-max_utilization util]
    [-pre_placement]
    [-buffer_gain float_value] (deprecated)
    [-match_cell_footprint]
    [-verbose]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-max_wire_length` | Maximum length of wires (in microns), defaults to a value that minimizes the wire delay for the wire RC values specified by `set_wire_rc`. |
| `-slew_margin` | Add a slew margin. The default value is `0`, the allowed values are integers `[0, 100]`. |
| `-cap_margin` | Add a capactitance margin. The default value is `0`, the allowed values are integers `[0, 100]`. |
| `-max_utilization` | Defines the percentage of core area used. |
| `-pre_placement` | Enables performing an initial pre-placement sizing and buffering round. |
| `-buffer_gain` | Deprecated alias for `-pre_placement`. The passed value is ignored. |
| `-match_cell_footprint` | Obey the Liberty cell footprint when swapping gates. |
| `-verbose` | Enable verbose logging on progress of the repair. |

### Repair Tie Fanout

The `repair_tie_fanout` command connects each tie high/low load to a copy
of the tie high/low cell.

```tcl
repair_tie_fanout 
    [-separation dist]
    [-max_fanout fanout]
    [-verbose]
    lib_port
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-separation` | Tie high/low insts are separated from the load by this value (Liberty units, usually microns). |
| `-verbose` | Enable verbose logging of repair progress. |
| `lib_port` | Tie high/low port, which can be a library/cell/port name or object returned by `get_lib_pins`. |

### Repair Timing

The `repair_timing` command repairs setup and hold violations.  It
should be run after clock tree synthesis with propagated clocks.
Setup repair is done before hold repair so that hold repair does not
cause setup checks to fail.

The worst setup path is always repaired.  Next, violating paths to
endpoints are repaired to reduced the total negative slack. 

```tcl
repair_timing 
    [-setup]
    [-hold]
    [-recover_power percent_of_paths_with_slack]
    [-setup_margin setup_margin]
    [-hold_margin hold_margin]
    [-slack_margin slack_margin]
    [-libraries libs]
    [-allow_setup_violations]
    [-skip_pin_swap]
    [-skip_gate_cloning]
    [-skip_buffering]
    [-skip_buffer_removal]
    [-skip_last_gasp]
    [-repair_tns tns_end_percent]
    [-max_passes passes]
    [-max_repairs_per_pass max_repairs_per_pass]
    [-max_utilization util]
    [-max_buffer_percent buffer_percent]
    [-match_cell_footprint]
    [-verbose]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-setup` | Repair setup timing. |
| `-hold` | Repair hold timing. |
| `-recover_power` | Set the percentage of paths to recover power for. The default value is `0`, and the allowed values are floats `(0, 100]`. |
| `-setup_margin` | Add additional setup slack margin. |
| `-hold_margin` | Add additional hold slack margin. |
| `-allow_setup_violations` | While repairing hold violations, buffers are not inserted that will cause setup violations unless `-allow_setup_violations` is specified. |
| `-skip_pin_swap` | Flag to skip pin swap. The default is to perform pin swap transform during setup fixing. |
| `-skip_gate_cloning` | Flag to skip gate cloning. The default is to perform gate cloning transform during setup fixing. |
| `-skip_buffering` | Flag to skip rebuffering and load splitting. The default is to perform rebuffering and load splitting transforms during setup fixing. |
| `-skip_buffer_removal` | Flag to skip buffer removal.  The default is to perform buffer removal transform during setup fixing. |
| `-skip_last_gasp` | Flag to skip final ("last gasp") optimizations.  The default is to perform greedy sizing at the end of optimization. |
| `-repair_tns` | Percentage of violating endpoints to repair (0-100). When `tns_end_percent` is zero, only the worst endpoint is repaired. When `tns_end_percent` is 100 (default), all violating endpoints are repaired. |
| `-max_repairs_per_pass` | Maximum repairs per pass, default is 1. On the worst paths, the maximum number of repairs is attempted. It gradually decreases until the final violations which only get 1 repair per pass. |
| `-max_utilization` | Defines the percentage of core area used. |
| `-max_buffer_percent` | Specify a maximum number of buffers to insert to repair hold violations as a percentage of the number of instances in the design. The default value is `20`, and the allowed values are integers `[0, 100]`. |
| `-match_cell_footprint` | Obey the Liberty cell footprint when swapping gates. |
| `-verbose` | Enable verbose logging of the repair progress. |

Use`-recover_power` to specify the percent of paths with positive slack which
will be considered for gate resizing to save power. It is recommended that
this option be used with global routing based parasitics. 

### Repair Clock Nets

The `clock_tree_synthesis` command inserts a clock tree in the design
but may leave a long wire from the clock input pin to the clock tree
root buffer.

The `repair_clock_nets` command inserts buffers in the
wire from the clock input pin to the clock root buffer.

```tcl
repair_clock_nets 
    [-max_wire_length max_wire_length]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-max_wire_length` | Maximum length of wires (in microns), defaults to a value that minimizes the wire delay for the wire RC values specified by `set_wire_rc`. |

### Repair Clock Inverters

The repair_clock_inverters command replaces an inverter in the clock
tree with multiple fanouts with one inverter per fanout.  This
prevents the inverter from splitting up the clock tree seen by CTS.
It should be run before clock_tree_synthesis.

```tcl
repair_clock_inverters
```

### Report Design Area

The `report_design_area` command reports the area of the design's components
and the utilization.

```tcl
report_design_area
```

### Report Floating Nets

The `report_floating_nets` command reports nets with connected loads but no connected drivers.

```tcl
report_floating_nets 
    [-verbose]
```

### Report Overdriven Nets

The `report_overdriven_nets` command reports nets with connected by multiple drivers.

```tcl
report_overdriven_nets
    [-include_parallel_driven]
    [-verbose]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-include_parallel_driven` | Include nets that are driven by multiple parallel drivers. |
| `-verbose` | Print the net names. |

### Eliminate Dead Logic

The `eliminate_dead_logic` command eliminates dead logic, i.e. it removes standard cell instances which can be removed without affecting the function of the design.

```tcl
eliminate_dead_logic
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-verbose` | Print the net names. |

## Useful Developer Commands

If you are a developer, you might find these useful. More details can be found in the [source file](./src/Resizer.cc) or the [swig file](./src/Resizer.i).

| Command Name | Description |
| ----- | ----- |
| `repair_setup_pin` | Repair setup pin violation. |
| `check_parasitics` | Check if the `estimate_parasitics` command has been called. |
| `parse_time_margin_arg` | Get the raw value for timing margin (e.g. `slack_margin`, `setup_margin`, `hold_margin`) |
| `parse_percent_margin_arg` | Get the above margin in perentage format. |
| `parse_margin_arg` | Same as `parse_percent_margin_arg`. |
| `parse_max_util` | Check maximum utilization. |
| `parse_max_wire_length` | Get maximum wirelength. |
| `check_corner_wire_caps` | Check wire capacitance for corner. |
| `check_max_wire_length` | Check if wirelength is allowed by rsz for minimum delay. |
| `dblayer_wire_rc` | Get layer RC values. |
| `set_dblayer_wire_rc` | Set layer RC values. |

### Setting Optimization Configuration

The `set_opt_config` command configures optimization settings that apply to
data cell selection, affecting all optimization commands like repair_design and repair_timing.
However, this does not apply to clock cell selection in clock_tree_synthesis or repair_clock_nets.

```tcl
set_opt_config 
    [-limit_sizing_area float_value]
    [-limit_sizing_leakage float_value]
    [-keep_sizing_site boolean_value]
    [-keep_sizing_vt boolean_value]
    [-set_early_sizing_cap_ratio float_value]
    [-set_early_buffer_sizing_cap_ratio float_value]
    [-sizing_area_limit float_value] (deprecated)
    [-sizing_leakage_limit float_value] (deprecated)
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-limit_sizing_area` | Exclude cells from sizing if their area exceeds <float_value> times the current cell's area. For example, with 2.0, only cells with an area <= 2X the current cell's area are considered. The area is determined from LEF, not Liberty. |
| `-limit_sizing_leakage` | Exclude cells from sizing if their leakage power exceeds <float_value> times the current cell's leakage. For example, with 2.0, only cells with leakage <= 2X the current cell's leakage are considered. Leakage power is based on the current timing corner. |
| `-keep_sizing_site` | Ensure cells retain their original site type during sizing. This prevents short cells from being replaced by tall cells (or vice versa) in mixed-row designs. |
| `-keep_sizing_vt` | Preserve the cell's VT type during sizing, preventing swaps between HVT and LVT cells. This works only if VT layers are defined in the LEF obstruction section. |
| `-set_early_sizing_cap_ratio` | Maintain the specified ratio between input pin capacitance and output pin load when performing initial sizing of gates. |
| `-set_early_buffer_sizing_cap_ratio` | Maintain the specified ratio between input pin capacitance and output pin load when performing initial sizing of buffers. |
| `-sizing_area_limit` | Deprecated.   Use -limit_sizing_area instead. |
| `-sizing_leakage_limit` | Deprecated.  Use -limit_sizing_leakage instead. |

### Reporting Optimization Configuration

The `report_opt_config` command reports current optimization configuration

```tcl
report_opt_config 
```

### Resetting Optimization Configuration

The `reset_opt_config` command resets optimization settings applied from `set_opt_config` command.
If no options are specified, all optimization configurations are reset.

```tcl
reset_opt_config 
    [-limit_sizing_area]
    [-limit_sizing_leakage]
    [-keep_sizing_site]
    [-keep_sizing_vt]
    [-set_early_sizing_cap_ratio]
    [-set_early_buffer_sizing_cap_ratio]
    [-sizing_area_limit] (deprecated)
    [-sizing_leakage_limit] (deprecated)
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-limit_sizing_area` | Remove area restriction during sizing. |
| `-limit_sizing_leakage` | Remove leakage power restriction during sizing. |
| `-keep_sizing_site` | Remove site restriction during sizing. |
| `-keep_sizing_vt` | Remove VT type restriction during sizing. |
| `-set_early_sizing_cap_ratio` | Remove capacitance ratio setting for early sizing. |
| `-set_early_buffer_sizing_cap_ratio` | Remove capacitance ratio setting for early buffer sizing. |
| `-sizing_area_limit` | Deprecated.  Use -limit_sizing_area instead. |
| `-sizing_leakage_limit` | Deprecated.  Use -limit_sizing_leakage instead. |

### Finding Equivalent Cells

The `report_equiv_cells` command finds all functionally equivalent library cells for a given library cell with relative area and leakage power details.

```tcl
report_equiv_cells 
    [-match_cell_footprint]
    [-all]
    lib_cell
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-match_cell_footprint` | Limit equivalent cell list to include only cells that match library cell_footprint attribute. |
| `-all` | List all equivalent cells, ignoring sizing restrictions and cell_footprint.  Cells excluded due to these restrictions are marked with an asterisk. |

## Example scripts

A typical `resizer` command file (after a design and Liberty libraries have
been read) is shown below.

```
read_sdc gcd.sdc

set_wire_rc -layer metal2

set_dont_use {CLKBUF_* AOI211_X1 OAI211_X1}

buffer_ports
repair_design -max_wire_length 100
repair_tie_fanout LOGIC0_X1/Z
repair_tie_fanout LOGIC1_X1/Z
# clock tree synthesis...
repair_timing
```

Note that OpenSTA commands can be used to report timing metrics before
or after resizing the design.

```
set_wire_rc -layer metal2
report_checks
report_tns
report_wns
report_checks

repair_design

report_checks
report_tns
report_wns
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests). 

Simply run the following script: 

```shell
./test/regression
```

## Limitations

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+resizer)
about this tool.

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
