# Detailed Routing

The Detailed Routing (`drt`) module in OpenROAD is based on the open-source
detailed router, TritonRoute. TritonRoute consists of several main 
building blocks, including pin access analysis, track assignment,
initial detailed routing,  search and repair, and a DRC engine.
The initial development of the
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

This command performs detailed routing.

Developer arguments
- `-or_seed`, `-or_k`

Distributed arguments
- `-distributed` , `-remote_host`, `-remote_port`, `-shared_volume`, `-cloud_size`

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
    [-or_k k]
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
    [-single_step_dr]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-output_maze` | Path to output maze log file (e.g. `output_maze.log`). |
| `-output_drc` | Path to output DRC report file (e.g. `output_drc.rpt`). |
| `-output_cmap` | Path to output congestion map file (e.g. `output.cmap`). |
| `-output_guide_coverage` | Path to output guide coverage file (e.g. `sample_coverage.csv`). |
| `-drc_report_iter_step` | Report DRC on each iteration which is a multiple of this step. The default value is `0`, and the allowed values are integers `[0, MAX_INT]`. |
| `-db_process_node` | Specify the process node. |
| `-disable_via_gen` | Option to diable via generation with bottom and top routing layer. The default value is disabled. | 
| `-droute_end_iter` | Number of detailed routing iterations. The default value is `-1`, and the allowed values are integers `[1, 64]`. |
| `-via_in_pin_bottom_layer` | Via-in pin bottom layer name. |
| `-via_in_pin_top_layer` | Via-in pin top layer name. |
| `-or_seed` | Refer to developer arguments [here](#developer-arguments). |
| `-or_k` | Refer to developer arguments [here](#developer-arguments). |
| `-bottom_routing_layer` | Bottommost routing layer name. |
| `-top_routing_layer` | Topmost routing layer name. |
| `-verbose` | Sets verbose mode if the value is greater than 1, else non-verbose mode (must be integer, or error will be triggered.) |
| `-distributed` | Refer to distributed arguments [here](#distributed-arguments). |
| `-clean_patches` | Clean unneeded patches during detailed routing. | 
| `-no_pin_access` | Disables pin access for routing. |
| `-min_access_points` | Minimum access points for standard cell and macro cell pins. | 
| `-save_guide_updates` | Flag to save guides updates. |
| `-repair_pdn_vias` | This option is used for PDKs where M1 and M2 power rails run in parallel. |

#### Developer arguments

Some arguments that are helpful for developers are listed here. 

| Switch Name | Description |
| ----- | ----- |
| `-or_seed` | Random seed for the order of nets to reroute. The default value is `-1`, and the allowed values are integers `[0, MAX_INT]`. | 
| `-or_k` | Number of swaps is given by $k * sizeof(rerouteNets)$. The default value is `0`, and the allowed values are integers `[0, MAX_INT]`. |

### Detailed Route Debugging

The following command and arguments are useful when debugging error
messages from `drt` and to understand its behavior.

```tcl
detailed_route_debug 
    [-pa]
    [-ta]
    [-dr]
    [-maze]
    [-net name]
    [-pin name]
    [-box x1 y1 x2 y2]
    [-iter iter]
    [-pa_markers]
    [-dump_dr]
    [-dump_dir dir]
    [-dump_last_worker]
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
| `-box` | Set the box for debugging given by lower left/upper right coordinates. |
| `-worker` | Debugs routes that pass through the point `{x, y}`. |
| `-iter` | Specifies the number of debug iterations. The default value is `0`, and the accepted values are integers `[0, MAX_INT`. |
| `-pa_markers` | Enable pin access markers. |
| `-dump_dr` | Filename for detailed routing dump. |
| `-dump_dir` | Directory for detailed routing dump. |
| `-pa_edge` | Enable visibility of pin access edges. |
| `-pa_commit` | Enable visibility of pin access commits. |
| `-write_net_tracks` | Enable writing of net track assigments. |

### Check Pin Access

This function checks pin access.

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
| `-distributed` | Refer to distributed arguments [here](#distributed-arguments). |

#### Distributed Arguments

We have compiled all distributed arguments in this section.

```{note}
Additional setup is required. Please refer to this [guide](./doc/Distributed.md).
```

| Switch Name | Description |
| ----- | ----- |
| `-distributed` | Enable distributed mode with Kubernetes and Google Cloud. |
| `-remote_host` | The host IP. |
| `-remote_port` | The value of the port to access from. |
| `-shared_volume` | The mount path of the nfs shared folder. |
| `-cloud_size` | The number of workers. |

## Useful Developer Commands

If you are a developer, you might find these useful. More details can be found in the [source file](./src/TritonRoute.cpp) or the [swig file](./src/TritonRoute.i).

| Command Name | Description |
| ----- | ----- |
| `detailed_route_set_default_via` | Set default via. |
| `detailed_route_set_unidirectional_layer` | Set unidirectional layer. |
| `step_dr` | Refer to function `detailed_route_step_drt`. | 
| `check_drc` | Refer to function `check_drc_cmd`. |



## Example scripts

Example script demonstrating how to run TritonRoute on a sample design of `gcd`
in the Nangate45 technology node.

```shell
# single machine example 
./test/gcd_nangate45.tcl

# distributed example
./test/gcd_nangate45_distributed.tcl
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
