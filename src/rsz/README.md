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
internals. It uses the buffer library cell defined by `-buffer_cell` if it is given.

```tcl
buffer_ports 
    [-inputs] 
    [-outputs] 
    [-max_utilization util]
    [-buffer_cell buf_cell]
    [-verbose]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-inputs`, `-outputs` | Insert a buffer between the input and load, output and load respectively. The default behavior is `-inputs` and `-outputs` set if neither is specified. |
| `-max_utilization` | Defines the percentage of core area used. |
| `-buffer_cell`     | Specifies the buffer cell type to be used. |
| `-verbose`         | Enable verbose logging. |

#### Instance Name Prefixes

`buffer_ports` uses the following prefixes for the buffer instances that it inserts:

| Instance Prefix | Purpose |
| ----- | ----- |
| input | Buffering primary inputs |
| output | Buffering primary outputs |

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

#### Instance Name Prefixes

`repair_design` uses the following prefixes for the buffer instances that it inserts:

| Instance Prefix | Purpose |
| ----- | ----- |
| fanout | Fixing max fanout |
| gain | Gain based buffering |
| load_slew | Fixing max transition violations |
| max_cap | Fixing max capacitance |
| max_length | Fixing max length |
| wire | Repairs load slew, length, and max capacitance violations in net wire segment |

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
    [-sequence]
    [-skip_pin_swap]
    [-skip_gate_cloning]
    [-skip_size_down]
    [-skip_buffering]
    [-skip_buffer_removal]
    [-skip_last_gasp]
    [-skip_vt_swap]
    [-skip_crit_vt_swap]
    [-repair_tns tns_end_percent]
    [-max_passes passes]
    [-max_iterations iterations]
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
| `-sequence` | Specify a particular order of setup timing optimizations. The default is "unbuffer,vt_swap,sizeup,swap,buffer,clone,split". Obeys skip flags also. |
| `-skip_pin_swap` | Flag to skip pin swap. The default is to perform pin swap transform during setup fixing. |
| `-skip_gate_cloning` | Flag to skip gate cloning. The default is to perform gate cloning transform during setup fixing. |
| `-skip_size_down` | Flag to skip gate down sizing. The default is to perform non-critical fanout gate down sizing transform during setup fixing. |
| `-skip_buffering` | Flag to skip rebuffering and load splitting. The default is to perform rebuffering and load splitting transforms during setup fixing. |
| `-skip_buffer_removal` | Flag to skip buffer removal.  The default is to perform buffer removal transform during setup fixing. |
| `-skip_last_gasp` | Flag to skip final ("last gasp") optimizations.  The default is to perform greedy sizing at the end of optimization. |
| `-skip_vt_swap` | Flag to skip threshold voltage (VT) swap optimizations.  The default is to perform VT swap optimization to improve timing QoR. |
| `-skip_crit_vt_swap` | Flag to skip critical threshold voltage (VT) swap optimizations at the end of optimization.  The default is to perform critical VT swap optimization to improve timing QoR beyond repairing just the worst path per each violating endpoint. |
| `-repair_tns` | Percentage of violating endpoints to repair (0-100). When `tns_end_percent` is zero, only the worst endpoint is repaired. When `tns_end_percent` is 100 (default), all violating endpoints are repaired. |
| `-max_repairs_per_pass` | Maximum repairs per pass, default is 1. On the worst paths, the maximum number of repairs is attempted. It gradually decreases until the final violations which only get 1 repair per pass. |
| `-max_utilization` | Defines the percentage of core area used. |
| `-max_iterations` | Defines the maximum number of iterations executed when repairing setup and hold violations. The default is `-1`, which disables the limit of iterations. |
| `-max_buffer_percent` | Specify a maximum number of buffers to insert to repair hold violations as a percentage of the number of instances in the design. The default value is `20`, and the allowed values are integers `[0, 100]`. |
| `-match_cell_footprint` | Obey the Liberty cell footprint when swapping gates. |
| `-verbose` | Enable verbose logging of the repair progress. |

Use`-recover_power` to specify the percent of paths with positive slack which
will be considered for gate resizing to save power. It is recommended that
this option be used with global routing based parasitics.

#### Instance Name Prefixes

`repair_timing` uses the following prefixes for the buffer and gate instances that it inserts:

| Instance Prefix | Purpose |
| ----- | ----- |
| clone | Gate cloning |
| hold | Hold fixing |
| rebuffer | Buffering for setup fixing |
| split | Split off non-critical loads behind a buffer to reduce load |

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
| `parse_time_margin_arg` | Get the raw value for timing margin (e.g. `slack_margin`, `setup_margin`, `hold_margin`) |
| `parse_percent_margin_arg` | Get the above margin in perentage format. |
| `parse_margin_arg` | Same as `parse_percent_margin_arg`. |
| `parse_max_util` | Check maximum utilization. |
| `parse_max_wire_length` | Get maximum wirelength. |
| `check_max_wire_length` | Check if wirelength is allowed by rsz for minimum delay. |

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
    [-disable_buffer_pruning boolean_value]
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
| `-disable_buffer_pruning` | Disable buffer pruning to improve hold fixing by not filtering out delay cells or slow buffers. |
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
    [-disable_buffer_pruning]
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
| `-disable_buffer_pruning` | Restore buffer pruning for optimization. |
| `-sizing_area_limit` | Deprecated.  Use -limit_sizing_area instead. |
| `-sizing_leakage_limit` | Deprecated.  Use -limit_sizing_leakage instead. |

### Finding Equivalent Cells

The `report_equiv_cells` command finds all functionally equivalent library cells for a given library cell with relative area and leakage power details.

```tcl
report_equiv_cells 
    [-match_cell_footprint]
    [-all]
    [-vt]
    lib_cell
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-match_cell_footprint` | Limit equivalent cell list to include only cells that match library cell_footprint attribute. |
| `-all` | List all equivalent cells, ignoring sizing restrictions and cell_footprint.  Cells excluded due to these restrictions are marked with an asterisk. |
| `-vt`  | List all threshold voltage (VT) equivalent cells such as HVT, RVT, LVT, SLVT. |

### Reporting Buffers

The `report_buffers` command reports all usable buffers to include for optimization.
Usable buffers are standard cell buffers that are not clock buffers, always on buffers,
level shifters, or buffers marked as dont-use.  VT type, cell site,
cell footprint and leakage are also reported.

```tcl
report_buffers
    [-filtered]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-filtered` | Report buffers after filtering based on threshold voltage, cell footprint, drive strength and cell site.  Subset of filtered buffers are used for rebuffering. |

### Optimizing Arithmetic Modules

The `replace_arith_modules` command optimizes design performance by intelligently swapping hierarchical arithmetic modules based on realistic timing models.
This command analyzes critical timing paths and replaces arithmetic modules with equivalent but architecturally different implementations to
improve Quality of Results (QoR) for the specified target.

#### Arithmetic Module Types

Yosys and OpenROAD support the following arithmetic module variants with different timing/area trade-offs.

ALU (Arithmetic Logic Unit) Variants

Han-Carlson (default)
: Balanced delay and area.  Best for general purpose applications.

Kogge-Stone
: Fastest, largest area.  Best for timing-constrained designs.

Brent-Kung
: Slower, smaller area.  Best for area-constrained designs.

Sklansky
: Moderate delay/area.  Best for balanced optimization.

MACC (Multiply-Accumulate) Variants

Booth (default)
: Balanced delay and area.  Best for general purpose applications.

Base (Han-Carlson)
: Fastest, potentially larger area.  Best for timing-constrained designs.

#### Requirements for Arithmetic Module Swap

1. Hierarchical netlist with arithmetic operators.  Yosys can produce such designs by enabling "wrapped operator synthesis".
In OpenROAD-flow-scripts, this can be done as follows:

cd OpenROAD-flow-scripts/flow

make SYNTH_WRAPPED_OPERATORS=1

This requires a Verilog netlist.  DEF netlist alone is not sufficient for hierarchical optimization.

2. Hierarchically linked design.  The design needs to be linked to preserve hierarchical boundaries.  For example,

link_design top -hier

read_db -hier

```tcl
replace_arith_modules 
    [-path_count num_critical_paths]
    [-slack_threshold float]
    [-target setup | hold | power | area]
```

#### Options

| Switch Name | Description |
| ----------- | ---------- |
| `-path_count`           | Number of critical paths to analyze to identify candidate arithmetic modules to swap. The default value is `1000`, and the allowed values are integers. |
| `-slack_threshold`      | Slack threshold in library time units.  Use positive values to include paths with small positive slack. The default value is `0.0`, and the allowed values are floats. |
| `-target`               | Optimization target. Valid types are `setup`, `hold`, `power`, `area`. Default type is `setup`, and the allowed value is string. |

#### Arguments

Setup
ALU: replace all candidate modules with Kogge-Stone (fastest)
MACC: replace all candidate modules with Base (fastest)

Hold
Not available yet

Power
Not available yet

Area
Not available yet

#### SEE ALSO

replace_hier_module

#### EXAMPLES

Arithmetic modules follow this naming convention per Yosys:

ALU_\<io_config\>_\<width\>_\<config\>_\<architecture\>

MACC_\<io_config\>_\<width\>_\<architecture\>

Examples:

ALU_20_0_25_0_25_unused_CO_X_HAN_CARLSON

ALU_20_0_25_0_25_unused_CO_X_KOGGE_STONE

ALU_20_0_25_0_25_unused_CO_X_BRENT_KUNG

ALU_25_0_20_0_25_unused_CO_X_SKLANSKY

\\MACC_14'10001011010100_19_BOOTH

\\MACC_14'10001011010100_19_BASE

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
#clock tree synthesis...
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
