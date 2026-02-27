# Global Routing

The global routing module in OpenROAD (`grt`) is based on FastRoute, an
open-source global router originally derived from Iowa State University's
FastRoute4.1 algorithm.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Global Route

This command performs global routing with the option to use a `guide_file`.
You may also choose to use incremental global routing using `-start_incremental`.

```tcl
global_route 
    [-guide_file out_file]
    [-congestion_iterations iterations]
    [-congestion_report_file file_name]
    [-congestion_report_iter_step steps]
    [-grid_origin {x y}]
    [-critical_nets_percentage percent]
    [-skip_large_fanout_nets fanout]
    [-allow_congestion]
    [-verbose]
    [-start_incremental]
    [-end_incremental]
    [-use_cugr]
    [-resistance_aware]
    [-infinite_cap]
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-guide_file` | Set the output guides file name (e.g., `route.guide`). |
| `-congestion_iterations` | Set the number of iterations made to remove the overflow of the routing. The default value is `50`, and the allowed values are integers `[0, MAX_INT]`. |
| `-congestion_report_file` | Set the file name to save the congestion report. The file generated can be read by the DRC viewer in the GUI (e.g., `report_file.rpt`). |
| `-congestion_report_iter_step` | Set the number of iterations to report. The default value is `0`, and the allowed values are integers `[0, MAX_INT]`. |
| `-grid_origin` | Set the (x, y) origin of the routing grid in DBU. For example, `-grid_origin {1 1}` corresponds to the die (0, 0) + 1 DBU in each x--, y- direction. |
| `-critical_nets_percentage` | Set the percentage of nets with the worst slack value that are considered timing critical, having preference over other nets during congestion iterations (e.g. `-critical_nets_percentage 30`). The default value is `0`, and the allowed values are integers `[0, MAX_INT]`. |
| `-skip_large_fanout_nets` | Skips routing for nets with a fanout higher than the specified limit. Nets above this pin count threshold are ignored by the global router and will not have routing guides, meaning they will also be skipped during detailed routing. This option is useful in debugging or estimation flows where high-fanout nets (such as pre-CTS clock nets) can be ignored. The default value is 0, indicating no fanout limit. The default value is `MAX_INT`. The allowed values are integers `[0, MAX_INT]`. |
| `-allow_congestion` | Allow global routing results to be generated with remaining congestion. The default is false. |
| `-verbose` | This flag enables the full reporting of the global routing. |
| `-start_incremental` | This flag initializes the GRT listener to get the net modified. The default is false. |
| `-end_incremental` | This flag run incremental GRT with the nets modified. The default is false. |
| `-use_cugr` | This flag run GRT using CUGR as the router solver. NOTE: this is not ready for production. |
| `-resistance_aware` | This flag enables resistance-aware layer assignment and 3D routing. NOTE: this is not ready for production. |
| `-infinite_cap` | Enables "infinite" gcell capacity for testing purpose. NOTE: this is not recommended for production flows. |

### Set Routing Layers

This command sets the minimum and maximum routing layers for signal and clock nets.
Example: `set_routing_layers -signal Metal2-Metal10 -clock Metal6-Metal9`

```tcl
set_routing_layers 
    [-signal min-max]
    [-clock min-max]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-signal` | Set the min and max routing signal layer (names) in this format "%s-%s". |
| `-clock` | Set the min and max routing clock layer (names) in this format "%s-%s". |

### Set Macro Extension

This command sets the halo (in terms of GCells) along the boundaries of macros.
A `GCell` is typically defined in terms of `Mx` routing tracks.
Example: `set_macro_extension 2`

```tcl
set_macro_extension extension
```

#### Options

| Argument Name | Description | 
| ----- | ----- |
| `extension` | Number of `GCells` added to the blockage boundaries from macros. The default `GCell` size is 15 `M3` pitches. |

### Set Pin Offset

This command sets the pin offset distance.

```tcl
set_pin_offset offset 
```

#### Options

| Argument Name | Description | 
| ----- | ----- |
| `offset` | Pin offset in microns (must be a positive integer). | 

### Set Global Routing Layer Adjustment

The `set_global_routing_layer_adjustment` command sets routing resource
adjustments in the routing layers of the design.  Such adjustments reduce the number of
routing tracks that the global router assumes to exist. This promotes the spreading of routing
and reduces peak congestion, to reduce challenges for detailed routing.

You can set adjustment for a
specific layer, e.g., `set_global_routing_layer_adjustment Metal4 0.5` reduces
the routing resources of routing layer `Metal4` by 50%.  You can also set adjustment
for all layers at once using `*`, e.g., `set_global_routing_layer_adjustment * 0.3` reduces the routing resources of all routing layers by 30%.  And, you can
also set resource adjustment for a layer range, e.g.: `set_global_routing_layer_adjustment
Metal4-Metal8 0.3` reduces the routing resources of routing layers  `Metal4`,
`Metal5`, `Metal6`, `Metal7` and `Metal8` by 30%.

Negative adjustment values can be used to increase the capacity of a given metal layer. For example,
`set_global_routing_layer_adjustment Metal5 -0.5` will increase the total capacity of `Metal5` by 50%.

```tcl
set_global_routing_layer_adjustment layer adjustment
```

#### Options

| Argument Name | Description | 
| ----- | ----- |
| `layer` | String for the layer name. |
| `adjustment` | Float indicating the percentage reduction of each edge in the specified layer. |


### Set Global Routing Region Adjustment

Set global routing region adjustment.
Example: `set_global_routing_region_adjustment {1.5 2 20 30.5} -layer Metal4 -adjustment 0.7`

```tcl
set_global_routing_region_adjustment
    {lower_left_x lower_left_y upper_right_x upper_right_y}
    -layer layer 
    -adjustment adjustment
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `lower_left_x`, `lower_left_y`, `upper_right_x` , `upper_right_y` | Bounding box to consider. |
| `-layer` | String for the layer name. |
| `-adjustment` | Float indicating the percentage reduction of each edge in the specified layer. |

### Set Global Routing Randomness

The command randomizes global routing by shuffling the order of the nets
and randomly subtracts or adds to the capacities of a random set of edges. 

Example:
`set_global_routing_random -seed 42 \
  -capacities_perturbation_percentage 50 \
  -perturbation_amount 2`

```tcl
set_global_routing_random 
    [-seed seed]
    [-capacities_perturbation_percentage percent]
    [-perturbation_amount value]
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-seed` | Sets the random seed (must be non-zero for randomization). |
| `-capacities_perturbation_percentage` | Sets the percentage of edges whose capacities are perturbed. By default, the edge capacities are perturbed by adding or subtracting 1 (track) from the original capacity.  |
| `-perturbation_amount` | Sets the perturbation value of the edge capacities. This option is only meaningful when `-capacities_perturbation_percentage` is used. |

### Set Specific Nets to Route

The `set_nets_to_route` command defines a list of nets to route. Only the nets
defined in this command are routed, leaving the remaining nets without any
global route guides.

```tcl
set_nets_to_route 
    net_names 
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `net_names` | Tcl list of set of nets (e.g. `{net1, net2}`). |

### Repair Antennas

The `repair_antennas` command checks the global routing for antenna
violations and repairs the violations by inserting diodes near the
gates of the violating nets.  By default the command runs only one
iteration to repair antennas. Filler instances added by the
`filler_placement` command should NOT be in the database when
`repair_antennas` is called. 

See LEF/DEF 5.8 Language Reference, Appendix C, "Calculating and
Fixing Process Antenna Violations" for a [description](coriolis.lip6.fr/doc/lefdef/lefdefref/lefdefref.pdf) 
of antenna violations.

If no `diode_cell` argument is specified the LEF cell with class CORE, ANTENNACELL will be used.
If any repairs are made the filler instances are remove and must be
placed with the `filler_placement` command.

If the LEF technology layer `ANTENNADIFFSIDEAREARATIO` properties are constant
instead of PWL, inserting diodes will not improve the antenna ratios, 
and thus, no
diodes are inserted. The following warning message will be reported:

```
[WARNING GRT-0243] Unable to repair antennas on net with diodes.
```

```tcl
repair_antennas 
    [diode_cell]
    [-iterations iterations]
    [-ratio_margin margin]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `diode_cell` | Diode cell to fix antenna violations. |
| `-iterations` | Number of iterations. The default value is `1`, and the allowed values are integers `[0, MAX_INT]`. |
| `-ratio_margin` | Add a margin to the antenna ratios. The default value is `0`, and the allowed values are integers `[0, 100]`. |


### Plot Global Routing Guides

The `draw_route_guides` command plots the route guides for a set of nets.
It also plots the route segments for a set of nets when using the -show_segments flag.
To erase the route guides from the GUI, pass an empty list to this command:
`draw_route_guides {}`.

```tcl
draw_route_guides 
    net_names
    [-show_segments]
    [-show_pin_locations]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `net_names` | Tcl list of set of nets (e.g. `{net1, net2}`). |
| `-show_segments` | Draw the route segments instead of the route guides. |
| `-show_pin_locations` | Draw circles for the pin positions on the routing grid. |

### Report Wirelength

The `report_wire_length` command reports the wire length of the nets. Use the `-global_route`
and the `-detailed_route` flags to report the wire length from global and detailed routing,
respectively. If none of these flags are used, the tool will identify the state of the design
and report the wire length accordingly.

Example: `report_wire_length -net {clk net60} -global_route -detailed_route -verbose -file out.csv`

```tcl
report_wire_length 
    [-net net_list]
    [-file file]
    [-global_route]
    [-detailed_route]
    [-verbose]
    [-summary]
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-net` | List of nets to report the wirelength. Use `*` to report the wire length for all nets of the design. |
| `-file` | The name of the file for the wirelength report. |
| `-global_route` | Report the wire length of the global routing. |
| `-detailed_route` | Report the wire length of the detailed routing. |
| `-verbose` | This flag enables the full reporting of the layer-wise wirelength information. |
| `-summary` | This flag reports the wire length for each layer of the design. |

### Global Route Debug Mode

The `global_route_debug` command allows you to start a debug mode to view the status of the Steiner Trees.
It also allows you to dump the input positions for the Steiner tree creation of a net.
This must be used before calling the `global_route` command. 
Set the name of the net and the trees that you want to visualize.

```tcl
global_route_debug 
    [-st]
    [-rst]
    [-tree2D]
    [-tree3D]
    [-saveSttInput file_name]
    [-net net_name]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-st` | Show the Steiner Tree generated by `stt`. |
| `-rst` | Show the Rectilinear Steiner Tree generated by `grt`. |
| `-tree2D` | Show the Rectilinear Steiner Tree generated by `grt` after the overflow iterations. |
| `-tree3D` | Show the 3D Rectilinear Steiner Tree post-layer assignment. |
| `-saveSttInput` | File name to save `stt` input of a net. |
| `-net` | The name of the net name to be displayed. |

### Read Global Routing Guides

This command reads global routing guides. 

```tcl
read_guides file_name 
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `file_name` | Path to global routing guide. | 

### Write Global Routing Segments

This command writes global routing segments, the raw routing data generated by
the global routing tool.

```tcl
write_global_route_segments file_name 
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `file_name` | Path to global routing segments file. | 

### Read Global Routing Segments

This command reads global routing segments, the raw routing data generated by
the global routing tool. Reading this format allows to perform parasitics
extraction, repair antennas and incremental routing over the input segments
file.

```tcl
read_global_route_segments file_name 
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `file_name` | Path to global routing segments file. | 

### Estimate Path Resistance Between Two Pins

This command calculates the path resistance between two pins considering the 
vias and wires connecting them. The two pins need to be connected by the same
wire.

```tcl
estimate_path_resistance pin_name_1 pin_name_2 [-verbose]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `pin_name_1` | Pin name 1 (e.g., `_437_/Y`). | 
| `pin_name_2` | Pin name 1 (e.g., `_438_/A`). |
| `-verbose` | Print path details. |

## Example scripts

Examples scripts demonstrating how to run FastRoute on a sample design of `gcd` as follows:

```shell
./test/gcd.tcl
```

## Useful Developer Commands

If you are a developer, you might find these useful. More details can be found in the [source file](./src/GlobalRouter.cpp) or the [swig file](./src/GlobalRouter.i).

| Command Name | Description |
| ----- | ----- |
| `check_routing_layer` | Check if the layer is within the min/max routing layer specified. |
| `parse_layer_name` | Get routing layer number from layer name |
| `parse_layer_range` | Parses a range from `layer_range` argument of format (%s-%s). `cmd` argument is not used. |
| `check_region` | Checks the defined region if its within the die area. |
| `have_detailed_route` | Checks if block has detailed route already. |

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests). 

Simply run the following script: 

```shell
./test/regression
```

## Limitations

## Using the Python interface to grt

```{warning}
The `Python` interface is currently in development and is subject to change.
```

The `Python` API tries to stay close to the API defined in the `C++` class
`GlobalRouter` that is located [here](./include/grt/GlobalRouter.h)

When initializing a design, a sequence of `Python` commands might look like
the following:

```python
from openroad import Design, Tech
tech = Tech()
tech.readLef(...)
design = Design(tech)
design.readDef(...)
gr = design.getGlobalRouter()
```    

Here are some options to the `global_route`
command. (See `GlobalRouter.h` for a complete list)

```python
gr.setGridOrigin(x, y)                     # int, default 0,0
gr.setCongestionReportFile(file_name)      # string
gr.setCongestionIterations(n)                # int, default 50
gr.setAllowCongestion(allowCongestion)     # boolean, default False
gr.setCriticalNetsPercentage(percentage)   # float
gr.setMinRoutingLayer(minLayer)            # int
gr.setMaxRoutingLayer(maxLayer)            # int
gr.setMinLayerForClock(minLayer)           # int
gr.setMaxLayerForClock(maxLayer)           # int
gr.setVerbose(v)                           # boolean, default False
```

and when ready to actually do the global route:

```python
gr.globalRoute(save_guides)                # boolean, default False
```    

If you have set `save_guides` to True, you can then save the guides in `file_name` with:

```python
design.getBlock().writeGuides(file_name)
```

You can find the index of a named layer with

```python
lindex = tech.getDB().getTech().findLayer(layer_name)
```

or, if you only have the `Python` design object

```python
lindex = design.getTech().getDB().getTech().findLayer(layer_name)
```    

Be aware that much of the error checking is done in `Tcl`, so that with
the current `C++` / `Python` API, that might be an issue to deal
with. There are also some useful `Python` functions located in the `grt_aux.py` [file](./test/grt_aux.py)
but these are not considered a part of the *final* API and may be subject to change.

## FAQs

Check out
[GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+fastroute+in%3Atitle)
about this tool.

## References

-   Database comes from [OpenDB](https://github.com/The-OpenROAD-Project/OpenDB)
-   [FastRoute 4.1 documentation](src/fastroute/README).  The FastRoute4.1
    version was received from [Yue Xu](mailto:yuexu@iastate.edu) on June 15, 2019.
-   Min Pan, Yue Xu, Yanheng Zhang and Chris Chu. "FastRoute: An Efficient and
    High-Quality Global Router. VLSI Design, Article ID 608362, 2012."
    Available [here](https://home.engineering.iastate.edu/~cnchu/pubs/j52.pdf).
-   C. J. Alpert, T. C. Hu, J. H. Huang, A. B. Kahng and
    D. Karger, "Prim-Dijkstra Tradeoffs for Improved Performance-Driven
    Global Routing", IEEE Transactions on Computer-Aided Design of
    Integrated Circuits and Systems 14(7) (1995), pp. 890-896. Available
    [here](https://vlsicad.ucsd.edu/Publications/Journals/j18.pdf).


## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
