# FastRoute

FastRoute is an open-source global router originally derived from Iowa State University's FastRoute4.1.

## Commands

```
global_route [-guide_file out_file]
             [-verbose verbose]
             [-congestion_iterations iterations]
             [-grid_origin {x y}]
             [-allow_congestion]

```

Options description:

-   `guide_file`: Set the output guides file name (e.g., -guide_file
    `route.guide`)
-   `verbose`: Set verbosity of reporting: 0 for less verbosity, 1 for medium
    verbosity, 2 for full verbosity (e.g., -verbose `1`)
-   `congestion_iterations`: Set the number of iterations made to remove the
    overflow of the routing (e.g., `-congestion_iterations 50`)
-   `grid_origin`: Set the (x, y) origin of the routing grid in DBU. For
    example, `-grid_origin {1 1}` corresponds to the die (0, 0) + 1 DBU in each
    x-, y- direction.
-   `allow_congestion`: Allow global routing results to be generated with remaining congestion

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

The `set_global_routing_random` command enables randomization of global routing
results. The randomized global routing shuffles the order of the nets and randomly
subtracts or adds to the capacities of a random set of edges.  The `-seed`
option sets the random seed and is required to enable the randomization mode. The
`-capacities_perturbation_percentage` option sets the percentage of edges
whose capacities are perturbed. By default, the edge capacities are perturbed by
adding or subtracting 1 (track) from the original capacity.  The `-perturbation_amount`
option sets the perturbation value of the edge capacities. This option
will only have meaning and effect when `-capacities_perturbation_percentage` is used.
The random seed must be different from 0 to enable randomization of the global routing.

Example: `set_global_routing_random -seed 42 -capacities_perturbation_percentage 50 -perturbation_amount 2`

```
repair_antennas diodeCellName/diodePinName [-iterations iterations]
```

The repair_antenna command evaluates the global routing results to find
antenna violations, and repairs the violations by inserting diodes. The
input for this command is the diode cell and its pin names, and a prescribed
number of
iterations. By default, the command runs only one iteration to repair
antennas.  It uses the  `antennachecker` tool to identify any nets with antenna
violations and, for each such net, the exact number of diodes necessary to fix the
antenna violation.

Example: `repair_antenna sky130_fd_sc_hs__diode_2/DIODE`

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

## Example scripts

## Regression tests

## Limitations

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
