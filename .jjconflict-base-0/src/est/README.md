# Parasitics estimation

Parasitics estimation commands are described below.

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

### Report Layer RC

The `report_layer_rc` command reports the layer resistance and capacitance values used
for parasitics estimation. These values were previously set with the `set_layer_rc`
command or they originate from the LEF.

```tcl
report_layer_rc
    [-corner corner]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-corner` | Process corner to report. |

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


## Useful Developer Commands

If you are a developer, you might find these useful. More details can be found in the [source file](./src/Resizer.cc) or the [swig file](./src/Resizer.i).

| Command Name | Description |
| ----- | ----- |
| `check_parasitics` | Check if the `estimate_parasitics` command has been called. |
| `check_corner_wire_caps` | Check wire capacitance for corner. |
| `dblayer_wire_rc` | Get layer RC values. |
| `set_dblayer_wire_rc` | Set layer RC values. |

## Example scripts

Examples scripts demonstrating how to run parasitics estimation on a sample design as follows:

```shell
./test/make_parasitics1.tcl
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
