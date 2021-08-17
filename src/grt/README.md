FastRoute
======================

**FastRoute** is an open-source global router.

The algorithm base is from FastRoute4.1, and the database comes from [OpenDB](https://github.com/The-OpenROAD-Project/OpenDB)


The FastRoute4.1 version was received from <yuexu@iastate.edu> on June 15, 2019, with the BSD-3 open source license as given in the FastRoute [website](http://home.eng.iastate.edu/~cnchu/FastRoute.html#License).

**Important**: This branch is used to compile with the OpenROAD app. For the standalone mode of FastRoute, check *master* branch.

## Commands

```
global_route [-guide_file out_file] \
             [-verbose verbose] \
             [-congestion_iterations iterations] \
             [-grid_origin {x y}] \
             [-allow_congestion]

```

Options description:
- ``guide_file``: Set the output guides file name (e.g.: -guide_file route.guide")
- ``verbose``: Set verbose of report. 0 for less verbose, 1 for medium verbose, 2 for full verbose (e.g.: -verbose `1`)
- ``congestion_iterations``: Set the number of iterations to remove the overflow of the routing (e.g.: -congestion_iterations `50`)
- ``grid_origin``: Set the origin of the routing grid (e.g.: -grid_origin {1 1})
- ``allow_congestion``: Allow global routing results with congestion

```
set_routing_layers [-signal min-max] \
                   [-clock min-max]
```
The `set_routing_layers` command sets the minimum and maximum routing layers for signal nets, with the `-signal` option,
and the the minimum and maximum routing layers for clock nets, with the `-clock` option
Example: `set_routing_layers -signal Metal2-Metal10 -clock Metal6-Metal9`

```
set_macro_extension extension
```
The `set_macro_extension` command sets the number of GCells added to the blocakges boundaries from macros
Example: `set_macro_extension 2`

```
set_global_routing_layer_adjustment layer adjustment
```
The `set_global_routing_layer_adjustment` command sets routing resources adjustments in the routing layers of the design.
You can set adjustment for a specific layer, e.g.: `set_global_routing_layer_adjustment Metal4 0.5` reduces the routing resources
of routing layer Metal4 in 50%.
You can set adjustment for all layers at once using `*`, e.g.: `set_global_routing_layer_adjustment * 0.3` reduces
the routing resources of all routing layers in 30%.
You can set adjustment for a layer range, e.g.: `set_global_routing_layer_adjustment Metal4-Metal8 0.3` reduces
the routing resources of routing layers  Metal4, Metal5, Metal6, Metal7 and Metal8 in 30%.

```
set_global_routing_layer_pitch layer pitch
```
The `set_global_routing_layer_pitch` command sets the pitch for routing tracks in a specific layer.
You can call it multiple times for different layers.
Example: `set_global_routing_layer_pitch Metal6 1.34`.

```
set_routing_alpha [-net net_name] alpha
```

By default the global router uses steiner trees to construct route guides. A
steiner tree minimizes the total wirelength.  Prim/Dijkstra is an alternative
net topology algorithm that supports a trade-off between total wirelength and
maximum path depth from the net driver to its loads.  The `set_routing_alpha`
command enables the Prim/Dijkstra algorithm and sets the alpha parameter used
to trade-off wirelength and path depth.  Alpha is between 0.0 and 1.0. When
alpha is 0.0 the net topology minimizes total wirelength (i.e. capacitance).
When alpha is 1.0 it minimizes longest path between the driver and loads
(i.e., maximum resistance).  Typical values are 0.4-0.8. For more information
about PDRev, check the paper in `src/FastRoute/src/pdrev/papers/PDRev.pdf` You
can call it multiple times for different nets.  Example: `set_routing_alpha
-net clk 0.3` sets the alpha value of 0.3 for net *clk*.

```
set_global_routing_region_adjustment {lower_left_x lower_left_y upper_right_x upper_right_y}
                                     -layer layer -adjustment adjustment
```

The `set_global_routing_region_adjustment` command sets routing resources
adjustments in a specific region of the design.  The region is defined as
a rectangle in a routing layer.

Example: `set_global_routing_region_adjustment {1.5 2 20 30.5} -layer Metal4 -adjustment 0.7`

```
set_global_routing_random [-seed seed] \
                          [-capacities_perturbation_percentage percent] \
                          [-perturbation_amount value]
```
The `set_global_routing_random` command enables random global routing results. The random global routing shuffles the order
of the nets and randomly subtracts or add the capacities of a random set of edges.
The `-seed` option sets the random seed and is required to enable random mode. The `-capacities_perturbation_percentage` option
sets the percentage of edges to perturb the capacities. By default, the edge capacities are perturbed by sum or subtract 1 from the original capacity.
The `-perturbation_amount` option sets the perturbation value of the edge capacities. This option will only have effect when `-capacities_perturbation_percentage`
is used.
The random seed must be different from 0 to enable random global routing.
Example: `set_global_routing_random -seed 42 -capacities_perturbation_percentage 50 -perturbation_amount 2`

```
repair_antennas diodeCellName/diodePinName [-iterations iterations]
```
The repair_antenna command evaluates the global routing results looking for antenna violations, and repairs the violations
by inserting diodes. The input for this command is the diode cell and pin names and the number of iterations. By default,
the command runs only one iteration to repair antennas.
It uses the  `antennachecker` tool to identify the antenna violations and return the exact number of diodes necessary to
fix the antenna violation.
Example: `repair_antenna sky130_fd_sc_hs__diode_2/DIODE`

```
write_guides file_name
```
The `write_guides` generates the guide file from the routing results.
Example: `write_guides route.guide`.

To estimate RC parasitics based on global route results, use the `-global_routing`
option of the `estimate_parasitics` command.

```
draw_route_guides net_names
```
The `draw_route_guides` command plots the route guides for a set of nets.
To erase the route guides from the GUI, pass an empty list to this command:
`draw_route_guides {}`.

```
estimate_parasitics -global_routing
```

# [License](LICENSE.md)

Copyright (c) 2019, Federal University of Rio Grande do Sul (UFRGS)
All rights reserved.
