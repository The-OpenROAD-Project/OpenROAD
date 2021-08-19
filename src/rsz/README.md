# Gate Resizer

Gate Resizer commands are described below.  The `resizer` commands stop when
the design area is `-max_utilization util` percent of the core area. `util`
is between 0 and 100.  The `resizer` stops and reports an error if the max
utilization is exceeded.

## Commands

### Set Wire RC

```
set_wire_rc [-clock] [-signal]
            [-layer layer_name]
            [-resistance res]
            [-capacitance cap]
```

The `set_wire_rc` command sets the resistance and capacitance used to estimate
delay of routing wires.  Separate values can be specified for clock and data
nets with the `-signal` and `-clock` flags. Without either `-signal` or
`-clock` the resistance and capacitance for clocks and data nets are set.
Use `-layer` or `-resistance` and `-capacitance`.  If `-layer` is used,
the LEF technology resistance and area/edge capacitance values for the
layer are used for a minimum-width wire on the layer.  The resistance and
capacitance values are per length of wire, not per square or per square micron.
The units for `-resistance` and `-capacitance` are from the first Liberty
file read, resistance_unit/distance_unit (typically kohms/micron) and
Liberty capacitance_unit/distance_unit (typically pf/micron or ff/micron).
If distance units are not specified in the Liberty file, microns are used.

### Set Layer RC

The `set_layer_rc` command can be used to set the resistance and capacitance
for a layer or via. This is useful if these values are missing from the LEF file,
or to override the values in the LEF.

```
set_layer_rc [-layer layer]
             [-via via_layer]
             [-capacitance cap]
             [-resistance res]
             [-corner corner]
```

For layers the resistance and capacitance units are the same as `set_wire_rc`
(per length of minimum-width wire). `layer` must be the name of a routing
layer.

Via resistance can also be set with the `set_layer_rc` command, using the
-via keyword.  `-capacitance` is not supported for vias. `via_layer` is
the name of a via layer.  Via resistance is per cut/via, not area based.

### Remove Buffers

```
remove_buffers
```

Use the `remove_buffers` command to remove buffers inserted by synthesis. This
step is recommended before using `repair_design` so that there is more flexibility
in buffering nets.

### Estimate Parasitics

```
estimate_parasitics -placement|-global_routing
```

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

### Set Don't Use

```
set_dont_use lib_cells
```

The `set_dont_use` command removes library cells from consideration by
the `resizer`. `lib_cells` is a list of cells returned by `get_lib_cells`
or a list of cell names (`wildcards` allowed). For example, `DLY*` says do
not use cells with names that begin with `DLY` in all libraries.

### Buffer Ports

```
buffer_ports [-inputs] [-outputs] [-max_utilization util]
```

The `buffer_ports -inputs` command adds a buffer between the input and its
loads.  The `buffer_ports -outputs` adds a buffer between the port driver
and the output port. The default behavior is `-inputs` and `-outputs`
if neither is specified.

### Repair Design

```
repair_design [-max_wire_length max_length]
              [-max_slew_margin slew_margin]
              [-max_cap_margin cap_margin]
              [-max_utilization util]
```

The `repair_design` command inserts buffers on nets to repair max slew, max
capacitance and max fanout violations, and on long wires to reduce RC delay in
the wire. It also resizes gates to normalize slews.  Use `estimate_parasitics
-placement` before `repair_design` to estimate parasitics considered
during repair. Placement-based parasitics cannot accurately predict
routed parasitics, so a margin can be used to "over-repair" the design
to compensate. Use `-max_slew_margin` to add a margin to the slews, and
`-max_cap_margin` to add a margin to the capacitances. Use `-max_wire_length`
to specify the maximum length of wires.  The maximum wirelength defaults to
a value that minimizes the wire delay for the wire resistance/capacitance
values specified by `set_wire_rc`.

### Set Max Fanout

Use the `set_max_fanout` SDC command to set the maximum fanout for the design.

```
set_max_fanout <fanout> [current_design]
```

### Repair Tie Fanout

```
repair_tie_fanout [-separation dist]
                  [-verbose]
                  lib_port
```

The `repair_tie_fanout` command connects each tie high/low load to a copy
of the tie high/low cell.  `lib_port` is the tie high/low port, which can
be a library/cell/port name or object returned by `get_lib_pins`. The tie
high/low instance is separated from the load by `dist` (in Liberty units,
typically microns).

### Repair Timing

```
repair_timing [-setup]
              [-hold]
              [-slack_margin slack_margin]
              [-allow_setup_violations]
              [-max_utilization util]
              [-max_buffer_percent buffer_percent]
```

The `repair_timing` command repairs setup and hold violations.  It should
be run after clock tree synthesis with propagated clocks.  While repairing
hold violations buffers are not inserted that will cause setup violations
unless '-allow_setup_violations' is specified.  Use `-slack_margin` to add
additional slack margin. To specify different slack margins use separate
`repair_timing` commands for setup and hold. Use `-max_buffer_percent` to
specify a maximum number of buffers to insert to repair hold violations as
a percentage of the number of instances in the design. The default value for
`buffer_percent` is 20, for 20%.

### Report Design Area

```
report_design_area
```

The `report_design_area` command reports the area of the design's components
and the utilization.

### Report Floating Nets

```
report_floating_nets [-verbose]
```

The `report_floating_nets` command reports nets with only one pin connection.
Use the `-verbose` flag to see the net names.

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

## Limitations

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+resizer+in%3Atitle)
about this tool.

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
