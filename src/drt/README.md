# Detailed Routing

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
| `-output_maze` | Path to output maze log file (e.g. `.output.maze.log`). |
| `-output_drc` | Path to output drc report file (e.g. `.output.drc.rpt`). |
| `-output_cmap` | Path to output congestion map file (e.g. `.output.cmap`). |
| `-output_guide_coverage` | Path to output guide coverage file (e.g. `_sample_coverage.csv`). |
| `-drc_report_iter_step` | Report drc on each iteration which is a multiple of this step (default 0, integer). |
| `-db_process_node` | Specify the process node. |
| `-disable_via_gen` | Option to diable via generation with bottom and top routing layer. (default not set) | 
| `-droute_end_iter` | Number of detailed routing iterations (must be a positive integer <= 64). |
| `-via_in_pin_bottom_layer` | Via-in pin bottom layer name. |
| `-via_in_pin_top_layer` | Via-in pin top layer name. |
| `-or_seed` | Random seed for the order of nets to reroute (default -1, integer). | 
| `-or_k` | Number of swaps is given by $k * sizeof(rerouteNets)$ (default 0, integer). |
| `-bottom_routing_layer` | Bottommost routing layer name. |
| `-top_routing_layer` | Topmost routing layer name. |
| `-verbose` | Sets verbose mode if the value is greater than 1, else non-verbose mode (must be integer, or error will be triggered.) |
| `-distributed` | Enable distributed mode with Kubernetes and Google Cloud, [guide](./doc/Distributed.md). |
| `-remote_host` | The host IP. |
| `-remote_port` | The value of the port to access from. |
| `-shared_volume` | The mount path of the nfs shared folder. |
| `-cloud_size` | The number of workers. |
| `-clean_patches` | Clean unneeded patches during detailed routing. | 
| `-no_pin_access` | Disables pin access for routing. |
| `-min_access_points` | Minimum access points for standard cell and macro cell pins. | 
| `-save_guide_updates` | Flag to save guides updates. |
| `-repair_pdn_vias` | This option is used for some PDKs which have M1 and M2 layers that run in parallel. |

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
| `-pa` | Enable debug for pin access. |
| `-ta` | Enable debug for track assignment. |
| `-dr` | Enable debug for detailed routing. |
| `-maze` | Enable debug for maze routing. | 
| `-net` | Enable debug for net name. |
| `-pin` | Enable debug for pin name. |
| `-worker` | Debugs routes that pass through the point `{x, y}`. |
| `-iter` | Specifies the number of debug iterations. (default 0, integer) |
| `-pa_markers` | Enable pin access markers. |
| `-dump_dr` | Filename for detailed routing dump. |
| `-dump_dir` | Directory for detailed routing dump. |
| `-pa_edge` | Enable visibility of pin access edges. |
| `-pa_commit` | Enable visibility of pin access commits. |
| `-write_net_tracks` | Enable writing of net track assigments. |

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
| `-db_process_node` | Specify process node. |
| `-bottom_routing_layer` | Bottommost routing layer. |
| `-top_routing_layer` | Topmost routing layer. |
| `-min_access_points` | Minimum number of access points per pin. |
| `-verbose` | Sets verbose mode if the value is greater than 1, else non-verbose mode (must be integer, or error will be triggered.) |
| `-distributed` | Enable distributed pin access algorithm. |
| `-remote_host` | The host IP. |
| `-remote_port` | The value of the port to access from. |
| `-shared_volume` | The mount path of the nfs shared folder. |
| `-cloud_size` | The number of workers. |

### Useful developer functions

If you are a developer, you might find these useful. More details can be found in the [source file](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.cpp) or the [swig file](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/drt/src/TritonRoute.i).

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

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests). 

Simply run the following script: 

```shell
./test/regression
```

## Limitations

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+tritonroute+in%3Atitle)
about this tool.

## References

Please cite the following paper(s) for publication:

1.   A. B. Kahng, L. Wang and B. Xu, "TritonRoute: The Open Source Detailed
    Router", IEEE Transactions on Computer-Aided Design of Integrated Circuits
    and Systems (2020), doi:10.1109/TCAD.2020.3003234. [(.pdf)](https://ieeexplore.ieee.org/ielaam/43/9358030/9120211-aam.pdf)
1.   A. B. Kahng, L. Wang and B. Xu, "The Tao of PAO: Anatomy of a Pin Access
    Oracle for Detailed Routing", Proc. ACM/IEEE Design Automation Conf., 2020,
    pp. 1-6. [(.pdf)](https://vlsicad.ucsd.edu/Publications/Conferences/377/c377.pdf)

## Authors

TritonRoute was developed by graduate students Lutong Wang and
Bangqi Xu from UC San Diego, and serves as the detailed router in the
[OpenROAD](https://theopenroadproject.org/) project.

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
