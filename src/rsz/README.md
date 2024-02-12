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

```tcl
set_wire_rc 
    [-clock] 
    [-signal]
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
| `-layer` | Use the LEF technology resistance and area/edge capacitance values for the layer. This is used for a default width wire on the layer. |
| `-resistance` | Resistance per unit length, units are from the first Liberty file read, usually in the form of $\frac{resistanceUnit}{distanceUnit}$. Usually kΩ/µm. |
| `-capacitance` | Capacitance per unit length, units are from the first Liberty file read, usually in the form of $\frac{capacitanceUnit}{distanceUnit}$. Usually pF/µm. |


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

```tcl
estimate_parasitics
    -placement|-global_routing
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-placement` or `-global_routing` | Either of these flags must be set. Parasitics are estimated based after placement stage versus after global routing stage. |

### Set Don't Use

The `set_dont_use` command removes library cells from consideration by
the `resizer` engine and the `CTS` engine. `lib_cells` is a list of cells returned by `get_lib_cells`
or a list of cell names (`wildcards` allowed). For example, `DLY*` says do
not use cells with names that begin with `DLY` in all libraries.

```tcl
set_dont_use lib_cells
unset_dont_use lib_cells
```

### Set Don't Touch

The `set_dont_touch` command prevents the resizer commands from
modifying instances or nets.

```tcl
set_dont_touch instances_nets
unset_dont_touch instances_nets
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
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-inputs`, `-outputs` | Insert a buffer between the input and load, output and load respectively. The default behavior is `-inputs` and `-outputs` set if neither is specified. |
| `-max_utilization` | Defines the percentage of core area used. |

### Remove Buffers

Use the `remove_buffers` command to remove buffers inserted by synthesis. This
step is recommended before using `repair_design` so that there is more flexibility
in buffering nets. 

```tcl
remove_buffers
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
    [-verbose]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-max_wire_length` | Maximum length of wires (in microns), defaults to a value that minimizes the wire delay for the wire RC values specified by `set_wire_rc`. |
| `-slew_margin` | Add a slew margin. The default value is `0`, the allowed values are integers `[0, 100]`. |
| `-cap_margin` | Add a capactitance margin. The default value is `0`, the allowed values are integers `[0, 100]`. |
| `-max_utilization` | Defines the percentage of core area used. |
| `-verbose` | Enable verbose logging on progress of the repair. |

### Repair Tie Fanout

The `repair_tie_fanout` command connects each tie high/low load to a copy
of the tie high/low cell.

```tcl
repair_tie_fanout 
    [-separation dist]
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
    [-allow_setup_violations]
    [-skip_pin_swap]
    [-skip_gate_cloning]
    [-repair_tns tns_end_percent]
    [-max_utilization util]
    [-max_buffer_percent buffer_percent]
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
| `-skip_pin_swap` | Flag to skip pin swap. The default value is `False`, and the allowed values are bools. |
| `-skip_gate_cloning` | Flag to skip gate cloning. The default value is `False`, and the allowed values are bools. |
| `-repair_tns` | Percentage of violating endpoints to repair (0-100). When `tns_end_percent` is zero (the default), only the worst endpoint is repaired. When `tns_end_percent` is 100, all violating endpoints are repaired. |
| `-max_utilization` | Defines the percentage of core area used. |
| `-max_buffer_percent` | Specify a maximum number of buffers to insert to repair hold violations as a percentage of the number of instances in the design. The default value is `20`, and the allowed values are integers `[0, 100]`. |
| `-verbose` | Enable verbose logging of the repair progress. |

Use`-recover_power` to specify the percent of paths with positive slack which
will be considered for gate resizing to save power. It is recommended that
this option be used with global routing based parasitics. 

### Repair Clock Nets

The `clock_tree_synthesis` command inserts a clock tree in the design
but may leave a long wire from the clock input pin to the clock tree
root buffer. The `repair_clock_nets` command inserts buffers in the
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

The `report_floating_nets` command reports nets with only one pin connection.

```tcl
report_floating_nets 
    [-verbose]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-verbose` | Print the net names. |

### Useful Developer Commands

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

## Example scripts

A typical `resizer` command file (after a design and Liberty libraries have
been read) is shown below.

```tcl
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

```tcl
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
