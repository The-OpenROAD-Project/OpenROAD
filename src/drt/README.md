# TritonRoute

TritonRoute is an open-source detailed router for modern industrial
designs. The router consists of several main building blocks, including
pin access analysis, track assignment, initial detailed routing,
search and repair, and a DRC engine. The initial development of the
[router](https://vlsicad.ucsd.edu/Publications/Conferences/363/c363.pdf)
is inspired by the [ISPD-2018 initial detailed routing
contest](http://www.ispd.cc/contests/18/).  However, the current framework
differs and is built from scratch, aiming for an industrial-oriented scalable
and flexible flow.

TritonRoute provides industry-standard LEF/DEF interface with
support of [ISPD-2018](http://www.ispd.cc/contests/18/) and
[ISPD-2019](http://www.ispd.cc/contests/19/) contest-compatible route
guide format.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Detailed Route

```tcl
detailed_route 
    [-output_maze filename]
    [-output_drc filename]
    [-output_cmap filename]
    [-output_guide_coverage filename]
    [-drc_report_iter_step step]
    [-db_process_node name]
    [-disable_via_gen]
    [-droute_end_iter iter]
    [-via_in_pin_bottom_layer layer]
    [-via_in_pin_top_layer layer]
    [-or_seed seed]
    [-or_k_ k]
    [-bottom_routing_layer layer]
    [-top_routing_layer layer]
    [-verbose level]
    [-distributed]
    [-remote_host rhost]
    [-remote_port rport]
    [-shared_volume vol]
    [-cloud_size sz]
    [-clean_patches]
    [-no_pin_access]
    [-min_access_points count]
    [-save_guide_updates]
    [-repair_pdn_vias layer]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-output_maze` | output maze log filename |
| `-output_drc` | output drc report filename |
| `-output_cmap` | output congestion map file |
| `-output_guide_coverage` | output guide coverage filename |
| `-drc_report_iter_step` | report drc on each iteration which is a multiple of this step (default 0) |
| `-db_process_node` | specify the process node |
| `-disable_via_gen` | option to diable via generation with bottom and top routing layer | 
| `-droute_end_iter` | number of detailed routing iterations, must be a positive integer <= 64 |
| `-via_in_pin_bottom_layer` | via-in pin bottom layer name |
| `-via_in_pin_top_layer` | via-in pin top layer name |
| `-or_seed` | random seed for the order of nets to reroute (default -1) | 
| `-or_k` | number of swaps is given by $k * sizeof(rerouteNets)$ (default 0) |
| `-bottom_routing_layer` | bottommost routing layer name |
| `-top_routing_layer` | topmost routing layer name |
| `-verbose` | set verbose if value is greater than 0 |
| `-distributed` | enable distributed mode with Kubernetes and Google Cloud, [guide](./doc/Distributed.md). |
| `-remote_host` | the host IP |
| `-remote_port` | the value of the port to access from |
| `-shared_volume` | the mount path of the nfs shared folder |
| `-cloud_size` | the number of workers |
| `-clean_patches` | clean unneeded patches during detailed routing. | 
| `-no_pin_access` | disables pin access for routing |
| `-min_access_points` | minimum access points for standard cell and macro cell pins. | 
| `-save_guide_updates` | save guides updates |
| `-repair_pdn_vias` | this option is used for some PDKs which have M1 and M2 layers that run in parallel. |

### Detailed Route debugging

```tcl
detailed_route_debug 
    [-pa]
    [-ta]
    [-dr]
    [-maze]
    [-net name]
    [-pin name]
    [-worker x y]
    [-iter iter]
    [-pa_markers]
    [-dump_dr]
    [-dump_dir dir]
    [-pa_edge]
    [-pa_commit]
    [-write_net_tracks]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-pa` | enable debug for pin access |
| `-ta` | enable debug for track assignment |
| `-dr` | enable debug for detailed routing |
| `-maze` | enable debug for maze routing | 
| `-net` | enable debug for net name |
| `-pin` | enable debug for pin name |
| `-worker` | debugs routes that pass through the point `{x, y}` |
| `-iter` | debug iterations |
| `-pa_markers` | enable pin access markers |
| `-dump_dr` | dump detailed routing filename |
| `-dump_dir` | dump detailed routing directory |
| `-pa_edge` | visibility of pin access edges |
| `-pa_commit` | visibility of pin access commits |
| `-write_net_tracks` | enable writing of net track assigments |

### Check Pin access 

```tcl
pin_access
    [-db_process_node name]
    [-bottom_routing_layer layer]
    [-top_routing_layer layer]
    [-min_access_points count]
    [-verbose level]
    [-distributed]
    [-remote_host rhost]
    [-remote_port rport]
    [-shared_volume vol]
    [-cloud_size sz]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-db_process_node` | specify for process node | 
| `-bottom_routing_layer` | bottommost routing layer |
| `-top_routing_layer` | topmost routing layer |
| `-min_access_points` | minimum number of access points per pin |
| `-verbose` | set verbose if the value is greater than 0 |
| `-distributed` | enable distributed pin access algorithm |
| `-remote_host` | the host IP |
| `-remote_port` | the value of the port to access from |
| `-shared_volume` | the mount path of the nfs shared folder |
| `-cloud_size` | the number of workers |

### Useful developer functions

If you are a developer, you might find these useful. More details can be found in the [source file](./src/TritonRoute.cpp) or the [swig file](./src/TritonRoute.i).

```
detailed_route_set_default_via [-via]   # set default via 
detailed_route_set_unidirectional_layer [-layer] # set unidirectional layer
step_dr # refer to function detailed_route_step_drt
check_drc   # refer to function check_drc_cmd 
```

## Example scripts

Example script demonstrating how to run TritonRoute on a sample design of `gcd`
in the Nangate45 technology node.

```shell
./test/gcd_nangate45.tcl    # single machine example
./test/gcd_nangate45_distributed.tcl    # distributed example
```

## Regression tests

There is a set of regression tests in `/test`.

```shell
./test/regression
```

## Limitations

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+tritonroute+in%3Atitle)
about this tool.

## References

Please cite the following paper(s) for publication:

-   A. B. Kahng, L. Wang and B. Xu, "TritonRoute: The Open Source Detailed
    Router", IEEE Transactions on Computer-Aided Design of Integrated Circuits
    and Systems (2020), doi:10.1109/TCAD.2020.3003234.
-   A. B. Kahng, L. Wang and B. Xu, "The Tao of PAO: Anatomy of a Pin Access
    Oracle for Detailed Routing", Proc. ACM/IEEE Design Automation Conf., 2020,
    pp. 1-6.

## Authors

TritonRoute was developed by graduate students Lutong Wang and
Bangqi Xu from UC San Diego, and serves as the detailed router in the
[OpenROAD](https://theopenroadproject.org/) project.

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
