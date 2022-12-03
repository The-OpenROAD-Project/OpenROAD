# FastRoute

FastRoute is an open-source global router originally derived from Iowa State University's FastRoute4.1.

## Commands

```
global_route [-guide_file out_file]
             [-congestion_iterations iterations]
             [-congestion_report_file file_name]
             [-grid_origin {x y}]
             [-critical_nets_percentage percent]
             [-allow_congestion]
             [-verbose]

```

Options description:

-   `guide_file`: Set the output guides file name (e.g., -guide_file
    `route.guide`).
-   `congestion_iterations`: Set the number of iterations made to remove the
    overflow of the routing (e.g., `-congestion_iterations 50`)
-   `congestion_report_file`: Set the file name to save congestion report. The 
    file generated can be read by DRC viewer in the gui (e.g., -congestion_report_file 
    `report_file.rpt`)
-   `grid_origin`: Set the (x, y) origin of the routing grid in DBU. For
    example, `-grid_origin {1 1}` corresponds to the die (0, 0) + 1 DBU in each
    x-, y- direction.
-   `critical_nets_percentage`: Set the percentage of nets with the worst slack value that are considered timing critical, having preference over other nets during congestion iterations (e.g. `-critical_nets_percentage 30`). The default percentage is 0%.
-   `allow_congestion`: Allow global routing results to be generated with remaining congestion.
-   `verbose`: This flag enables the full reporting of the global routing.

```
set_routing_layers [-signal min-max]
                   [-clock min-max]
```

The `set_routing_layers` command sets the minimum and maximum routing
layers for signal nets, with the `-signal` option, and the minimum and
maximum routing layers for clock nets, with the `-clock` option.

Example: `set_routing_layers -signal Metal2-Metal10 -clock Metal6-Metal9`

```
set_macro_extension extension
```

The `set_macro_extension` command sets the number of `GCells` added to the
blockages boundaries from macros. A `GCell` is typically defined in terms of
`Mx` routing tracks.  The default `GCell` size is 15 `M3` pitches.

Example: `set_macro_extension 2`

```
set_global_routing_layer_adjustment layer adjustment
```

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

```
set_routing_alpha [-net net_name] alpha
```

By default the global router uses heuristic rectilinear Steiner minimum
trees (RSMTs) as an initial basis to construct route guides. An RSMT
tries to minimize the total wirelength needed to connect a given set
of pins.  The Prim-Dijkstra heuristic is an alternative net topology
algorithm that supports a trade-off between total wirelength and maximum
path depth from the net driver to its loads. The `set_routing_alpha`
command enables the Prim/Dijkstra algorithm and sets the alpha parameter
used to trade-off wirelength and path depth.  Alpha is between 0.0
and 1.0. When alpha is 0.0 the net topology minimizes total wirelength
(i.e. capacitance).  When alpha is 1.0 it minimizes longest path between
the driver and loads (i.e., maximum resistance).  Typical values are
0.4-0.8. For more information about PDRev, check the paper
[here](in https://vlsicad.ucsd.edu/Publications/Journals/j18.pdf).
You can call it multiple times for different nets.

Example: `set_routing_alpha -net clk 0.3` sets the alpha value of 0.3 for net *clk*.

```
set_global_routing_region_adjustment {lower_left_x lower_left_y upper_right_x upper_right_y}
                                     -layer layer -adjustment adjustment
```

The `set_global_routing_region_adjustment` command sets routing resource
adjustments in a specific region of the design.  The region is defined as
a rectangle in a routing layer.

Example: `set_global_routing_region_adjustment {1.5 2 20 30.5} -layer Metal4 -adjustment 0.7`

```
set_global_routing_random [-seed seed]
                          [-capacities_perturbation_percentage percent]
                          [-perturbation_amount value]
```

The `set_global_routing_random` command enables randomization of
global routing results. The randomized global routing shuffles the
order of the nets and randomly subtracts or adds to the capacities of
a random set of edges.  The `-seed` option sets the random seed.  A
non-zero seed enables randomization. The
`-capacities_perturbation_percentage` option sets the percentage of
edges whose capacities are perturbed. By default, the edge capacities
are perturbed by adding or subtracting 1 (track) from the original
capacity.  The `-perturbation_amount` option sets the perturbation
value of the edge capacities. This option is only meaningful when
`-capacities_perturbation_percentage` is used.

Example:
`set_global_routing_random -seed 42 \
  -capacities_perturbation_percentage 50 \
  -perturbation_amount 2`

```
repair_antennas [diode_cell]
                [-iterations iterations]
                [-ratio_margin margin]
```

The repair_antenna command checks the global routing for antenna
violations and repairs the violations by inserting diodes near the
gates of the violating nets.  By default the command runs only one
iteration to repair antennas. Filler instances added by the
`filler_placement` command should NOT be in the database when
`repair_antennas` is called. Use `-ratio_margin` to add a margin
to the antenna ratios. `-ratio_margin` is between 0 and 100.

See LEF/DEF 5.8 Language Reference, Appendix C, "Calculating and
Fixing Process Antenna Violations" for a description of antenna
violations.

Example: `repair_antennas`

If no diode_cell argument is specified the LEF cell with
class CORE ANTENNACELL will be used.
If any repairs are made the filler instances are remove and must be
placed with the `filler_placement` command.

If the LEF technology layer ANTENNADIFFSIDEAREARATIO properties are constant
instead of PWL, inserting diodes does not improve the antenna ratios and no
diodes are inserted. The following error message will be reported:

[WARNING GRT-0243] Unable to repair antennas on net with diodes.

```
write_guides file_name
```
The `write_guides` generates the guide file from the routing results.

Example: `write_guides route.guide`.

To estimate RC parasitics based on global route results, use the `-global_routing`
option of the `estimate_parasitics` command.

```
estimate_parasitics -global_routing
```

```
draw_route_guides net_names [-show_pin_locations]
```
The `draw_route_guides` command plots the route guides for a set of nets.
To erase the route guides from the GUI, pass an empty list to this command:
`draw_route_guides {}`.
The `show_pin_locations` flag draw circles for the pin positions on the routing grid.

## Report wire length

```
report_wire_length [-net net_list]
                   [-file file]
                   [-global_route]
                   [-detailed_route]
                   [-verbose]
```

Options description:

-   `net`: A list of nets. Use `*` to report the wire length for all nets of the design.
-   `file`: The name of the file for the wire length report.
-   `global_route`: This flag is used to report the wire length of the global routing.
-   `detailed_route`: This flag is used to report the wire length of the detailed routing.
-   `verbose`: This flag enables the full reporting of the wire length information.

The `report_wire_length` command reports the wire length of the nets. Use the `-global_route`
and the `-detailed_route` flags to report the wire length from global and detailed routing,
respectively. If none of these flags are used, the tool will identify the state of the design
and report the wire length accordingly.


Example: `report_wire_length -net {clk net60} -global_route -detailed_route -verbose -file out.csv`

## Debug Mode

```
global_route_debug [-st]
                   [-rst]
                   [-tree2D]
                   [-tree3D]
                   [-saveSttInput file_name]
                   [-net net_name]

```
Options description:

-   `st`: Show the Steiner Tree generated by stt.
-   `rst`: Show the Rectilinear Steiner Tree generated by FastRoute.
-   `tree2D`: Show the Rectilinear Steiner Tree generated by FastRoute after the overflow iterations.
-   `tree3D`: Show the 3D Rectilinear Steiner Tree post-layer assignment.
-   `saveSttInput`: Set the file name to save stt input of a net.
-   `net`: Set the name of the net name to be displayed.

The `global_route_debug` command allows you to start a debug mode to view the status of the Steiner Trees. It also allows you to dump the input positions for the Steiner tree creation of a net. This must be used before calling the `global_route` command. Set the name of the net and the trees that you want to visualize.

## Example scripts

## Regression tests

## Limitations

## Using the Python interface to grt

**NOTE:** The Python interface is currently in development and may
change.

This api tries to stay close to the api defined in C++ class
GlobalRouter that is located in grt/include/grt/GlobalRouter.h. 

When initializing a design, a sequence of Python commands might look like
the following:

    from openroad import Design, Tech
    tech = Tech()
    tech.readLef(...)
    design = Design(tech)
    design.readDef(...)
    gr = design.getGlobalRouter()
    
Here are some options / configurations to the globalRoute
command. (See GlobalRouter.h for a complete list)

    gr.setGridOrigin(x, y)                     # int, default 0,0
    gr.setCongestionReportFile(file_name)      # string
    gr.setOverflowIterations(n)                # int, default 50
    gr.setAllowCongestion(allowCongestion)     # boolean, default False
    gr.setCriticalNetsPercentage(percentage)   # float
    gr.setMinRoutingLayer(minLayer)            # int
    gr.setMaxRoutingLayer(maxLayer)            # int
    gr.setMinLayerForClock(minLayer)           # int
    gr.setMaxLayerForClock(maxLayer)           # int
    gr.setVerbose(v)                           # boolean, default False

and when ready to actually do the global route:

    gr.globalRoute(save_guides)                # boolean, default False
    
If you have set `save_guides` to True, you can then save the guides in `file_name` with:

    design.getBlock().writeGuides(file_name)

You can find the index of a named layer with

    lindex = tech.getDB().getTech().findLayer(layer_name)

or, if you only have the Python design object

    lindex = design.getTech().getDB().getTech().findLayer(layer_name)
    
Be aware that much of the error checking is done in TCL, so that with
the current C++ / Python api, that might be an issue to deal
with. There are also some useful Python functions located in the file
grt/test/grt_aux.py but these are not considered a part of the (final)
api and they may change.

## FAQs

Check out
[GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+fastroute+in%3Atitle)
about this tool.

## External references

-   The algorithm base is from FastRoute4.1, and the database comes from
    [OpenDB](https://github.com/The-OpenROAD-Project/OpenDB)
-   [FastRoute 4.1 documentation](src/fastroute/README).  The FastRoute4.1
    version was received from <yuexu@iastate.edu> on June 15, 2019,
    with the BSD-3 open source license as given in the FastRoute
    [website](http://home.eng.iastate.edu/~cnchu/FastRoute.html#License).
-   Min Pan, Yue Xu, Yanheng Zhang and Chris Chu. "FastRoute: An Efficient and
    High-Quality Global Router. VLSI Design, Article ID 608362, 2012."
    Available [here](http://home.eng.iastate.edu/~cnchu/pubs/j52.pdf).
-   P-D paper is C. J. Alpert, T. C. Hu, J. H. Huang, A. B. Kahng and
    D. Karger, "Prim-Dijkstra Tradeoffs for Improved Performance-Driven
    Global Routing", IEEE Transactions on Computer-Aided Design of
    Integrated Circuits and Systems 14(7) (1995), pp. 890-896. Available
    [here](https://vlsicad.ucsd.edu/Publications/Journals/j18.pdf).


## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
