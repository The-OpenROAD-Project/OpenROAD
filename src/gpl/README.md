# Global Placement

The global placement module in OpenROAD (`gpl`) is based on the open-source
RePlAce tool, from the paper "Advancing Solution Quality and Routability Validation
in Global Placement".

Features:

-   Analytic and nonlinear placement algorithm. Solves
    electrostatic force equations using Nesterov's method.
    ([link](https://cseweb.ucsd.edu/~jlu/papers/eplace-todaes14/paper.pdf))
-   Verified with various commercial technologies and research enablements using OpenDB
    (7/14/16/28/45/55/65nm).
-   Verified deterministic solution generation with various compilers and OS.
-   Supports Mixed-size placement mode.

| <img src="./doc/image/adaptec2.inf.gif" width=350px> | <img src="./doc/image/coyote_movie.gif" width=400px> |
|:--:|:--:|
| Visualized examples from ISPD 2006 contest; adaptec2.inf |Real-world Design: Coyote (TSMC16 7.5T) |

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Global Placement

When using the `-timing_driven` flag, `gpl` does a virtual `repair_design` 
to find slacks and
weight nets with low slack. It adjusts the worst slacks (modified with 
`-timing_driven_nets_percentage`) using a multiplier (modified with 
`-timing_driven_net_weight_max`). The multiplier
is scaled from the full value for the worst slack, to 1.0 at the
`timing_driven_nets_percentage` point. Use the `set_wire_rc` command to set
resistance and capacitance of estimated wires used for timing. 

Timing-driven iterations are triggered based on a list of overflow threshold 
values. Each time the placer execution reaches these overflow values, the 
resizer is executed. This process can be costly in terms of runtime. The 
overflow values for recalculating weights can be modified with 
`-timing_driven_net_reweight_overflow`, you may use less overflow threshold 
values to decrease runtime, for example.

You can also set an overflow value for `keep_resize_below_overflow`, when below that, the modifications made by the rsz tool are maintained (non-virtual `repair_design`).

When the routability-driven option is enabled, each of its iterations will 
execute RUDY to provide an estimation of routing congestion. Congested tiles 
will have the area of their logic cells inflated to reduce routing congestion. 
The iterations will attempt to achieve the target RC (routing congestion) 
by comparing it to the final RC at each iteration. If the algorithm takes too 
long during routability-driven execution, consider raising the target RC value 
(`-routability_target_rc_metric`) to alleviate the constraints. The final RC 
value is calculated based on the weight coefficients. The algorithm will stop 
if the RC is not decreasing for three consecutive iterations.

The switches are generated from the fields of `gpl::PlaceOptions`
(`src/gpl/include/gpl/Replace.h`). `-options` takes the whole struct as a
nested dict (`default_options global_placement` returns the defaults to edit),
and there is one switch per field, named by the field's
dotted path (`-timing.nets_percentage` for
`timing.nets_percentage`), a flag for a bool (`-no_<field>` for a bool that is on by default, `-group.no_<field>` inside a group). The fields of the embedded `repair_design` and `global_route`
options are switches too (`-timing.repair_design.slew_margin`). The older
switch names listed under [Aliases](#aliases) remain accepted. The same struct,
with the same names, defaults and checks, is the Python `gpl.PlaceOptions`
class.

```tcl
global_placement
    [-skip_nesterov_place]
    [-incremental]
    [-options dict]
    [-density density]
    [-uniform_density]
    [-bin_grid_count_x bin_grid_count_x]
    [-bin_grid_count_y bin_grid_count_y]
    [-pad_left pad_left]
    [-pad_right pad_right]
    [-skip_io]
    [-place_ios]
    [-no_pin_density_adjust]
    [-routing_congestion]
    [-initial.max_iter max_iter]
    [-initial.min_diff_length min_diff_length]
    [-initial.max_solver_iter max_solver_iter]
    [-initial.max_fanout max_fanout]
    [-initial.net_weight_scale net_weight_scale]
    [-initial.force_center]
    [-initial.perturbation_seed perturbation_seed]
    [-initial.perturbation_dist perturbation_dist]
    [-nesterov.max_iter max_iter]
    [-nesterov.target_overflow target_overflow]
    [-nesterov.init_density_penalty init_density_penalty]
    [-nesterov.init_wirelength_coef init_wirelength_coef]
    [-nesterov.min_phi_coef min_phi_coef]
    [-nesterov.max_phi_coef max_phi_coef]
    [-nesterov.reference_hpwl reference_hpwl]
    [-nesterov.no_revert_if_diverge]
    [-timing.enabled]
    [-timing.reweight_overflows reweight_overflows]
    [-timing.net_weight_max net_weight_max]
    [-timing.nets_percentage nets_percentage]
    [-timing.keep_resize_below_overflow keep_resize_below_overflow]
    [-timing.repair_timing]
    [-timing.repair_tns_end_percent repair_tns_end_percent]
    [-timing.repair_design.max_wire_length max_wire_length]
    [-timing.repair_design.slew_margin slew_margin]
    [-timing.repair_design.cap_margin cap_margin]
    [-timing.repair_design.max_utilization max_utilization]
    [-timing.repair_design.pre_placement]
    [-timing.repair_design.match_cell_footprint]
    [-timing.repair_design.reroute]
    [-timing.repair_design.verbose]
    [-virtual_cts.enabled]
    [-virtual_cts.max_skew_fraction max_skew_fraction]
    [-routability.enabled]
    [-routability.no_use_rudy]
    [-routability.check_overflow check_overflow]
    [-routability.snapshot_overflow snapshot_overflow]
    [-routability.max_density max_density]
    [-routability.target_rc_metric target_rc_metric]
    [-routability.inflation_ratio_coef inflation_ratio_coef]
    [-routability.max_inflation_ratio max_inflation_ratio]
    [-routability.min_congestion_for_inflation min_congestion_for_inflation]
    [-routability.max_inflation_total max_inflation_total]
    [-routability.net_weight_max net_weight_max]
    [-routability.congested_nets_percentage congested_nets_percentage]
    [-routability.rc_coefficients rc_coefficients]
    [-routability.global_route.no_allow_congestion]
    [-routability.global_route.congestion_iterations congestion_iterations]
    [-routability.global_route.congestion_report_iter_step congestion_report_iter_step]
    [-routability.global_route.congestion_report_file congestion_report_file]
    [-routability.global_route.grid_origin grid_origin]
    [-routability.global_route.critical_nets_percentage critical_nets_percentage]
    [-routability.global_route.res_aware_nets_percentage res_aware_nets_percentage]
    [-routability.global_route.resistance_aware]
    [-routability.global_route.snapshot_batched_width snapshot_batched_width]
    [-routability.global_route.skip_large_fanout_nets skip_large_fanout_nets]
    [-routability.global_route.infinite_capacity]
    [-routability.global_route.verbose]
    [-routability.global_route.use_cugr]
    [-routability.global_route.save_guides]
```

#### Options

`describe_options global_placement` prints this table (with `-markdown`);
`report_options global_placement <switches>` shows the values a run would use.
A unique prefix of a switch is accepted. Switches and `-options` dicts combine
left to right, later ones overriding:

<!-- checker: skip -->
```tcl
global_placement -options {density 0.72 timing {enabled true nets_percentage 15}}
global_placement -options $opts -timing.nets_percentage 20
```

<!-- options-table: global_placement -->
| Switch Name | Type | Default | Description |
| ----- | ----- | ----- | ----- |
| `-options` | dict |  | The options as a nested dict of the fields below, or dotted keys (timing.nets_percentage); a switch after it overrides it. |
| `-density` | float (in [0, 1]) | `0.7` | Target density; 0.7 is 70%. |
| `-uniform_density` | flag |  | Use the minimum feasible target density instead of density. |
| `-bin_grid_count_x` | int (>= 0) | `0` | Bin grid columns; 0 lets the placer choose. Powers of two from 64 work best. |
| `-bin_grid_count_y` | int (>= 0) | `0` | Bin grid rows; 0 lets the placer choose. |
| `-pad_left` | int (>= 0) | `0` | Left padding in sites. |
| `-pad_right` | int (>= 0) | `0` | Right padding in sites. |
| `-skip_io` | flag |  | Ignore the IO pins when computing wirelength. |
| `-place_ios` | flag |  | Place the movable IO pins concurrently with the cells, along the die perimeter. Not supported on a rectilinear die. |
| `-no_pin_density_adjust` | flag |  | Grow the area of pin-dense instances. |
| `-routing_congestion` | flag |  | Run the global router after placement, for the routing congestion heatmap. |
| `-initial.max_iter` | int (>= 0) | `20` | Iterations of the quadratic (BiCGSTAB) initial placement; 0 skips it. Initial placement improves HPWL by ~5% on large designs. |
| `-initial.min_diff_length` | int (> 0) | `1500` | Minimum length change that counts as progress. |
| `-initial.max_solver_iter` | int (> 0) | `100` | Iterations of the linear solver per placement iteration. |
| `-initial.max_fanout` | int (> 0) | `200` | Nets with more pins are ignored. |
| `-initial.net_weight_scale` | float (> 0) | `800` | Scale of the net weights. |
| `-initial.force_center` | flag |  | Start from the core (or region) center even when the instances already have locations. |
| `-initial.perturbation_seed` | int (>= 0) | `1` | Seed of the random perturbation of the initial locations. |
| `-initial.perturbation_dist` | float | `-1` | Distance of the random perturbation in DBU; -1 for the internal default. |
| `-nesterov.max_iter` | int (> 0) | `5000` | Maximum Nesterov iterations. |
| `-nesterov.target_overflow` | float (in [0, 1]) | `0.1` | Overflow at which the placement stops. |
| `-nesterov.init_density_penalty` | float (> 0) | `8e-05` | Initial density penalty. |
| `-nesterov.init_wirelength_coef` | float (> 0) | `0.25` | Initial wirelength coefficient. |
| `-nesterov.min_phi_coef` | float (> 0) | `0.95` | Lower bound of the step scaling (pcof_min); 0.95-1.05 works well. |
| `-nesterov.max_phi_coef` | float (> 0) | `1.05` | Upper bound of the step scaling (pcof_max); 1.00-1.20 works well. |
| `-nesterov.reference_hpwl` | float (>= 0) | `0` | Wirelength change the density penalty controller measures against, in DBU; 0 derives it from the design. |
| `-nesterov.no_revert_if_diverge` | flag |  | Keep snapshots and revert to the last one when the placement diverges. |
| `-timing.enabled` | flag |  | Reweight the nets with the worst slack, found by a virtual repair_design. |
| `-timing.reweight_overflows` | list of int (> 0) | `{64 20}` | Overflows at which the slacks are recomputed and the nets reweighted; fewer values cost less runtime. |
| `-timing.net_weight_max` | float (> 0) | `5` | Weight of the net with the worst slack; the weight falls to 1 at nets_percentage. |
| `-timing.nets_percentage` | float (in [0, 100]) | `10` | Share of the nets, from the worst slack, that are reweighted. |
| `-timing.keep_resize_below_overflow` | float (in [0, 1]) | `1` | Below this overflow the resizer's changes are kept (non-virtual repair_design). |
| `-timing.repair_timing` | flag |  | Also run a conservative repair_timing when finding the slacks. |
| `-timing.repair_tns_end_percent` | float (in [0, 100]) | `1` | TNS end percent of that repair_timing. |
| `-timing.repair_design.max_wire_length` | float, distance (>= 0) | `0` | Maximum wire length; 0 picks the length that minimizes the wire delay for the set_wire_rc values. |
| `-timing.repair_design.slew_margin` | float (in [0, 100]) | `0` | Slew margin in percent, to over-repair for the difference between estimated and routed parasitics. |
| `-timing.repair_design.cap_margin` | float (in [0, 100]) | `0` | Capacitance margin in percent. |
| `-timing.repair_design.max_utilization` | float (in [0, 100]) | `none` | Percentage of the core area that may be used. |
| `-timing.repair_design.pre_placement` | flag |  | Run an initial sizing and buffering round before placement. |
| `-timing.repair_design.match_cell_footprint` | flag |  | Obey the Liberty cell footprint when swapping gates. |
| `-timing.repair_design.reroute` | flag |  | Resistance-aware rerouting of nets with slew violations after global routing (experimental). |
| `-timing.repair_design.verbose` | flag |  | Report the repair progress. |
| `-virtual_cts.enabled` | flag |  | Model the clock insertion delay per sink with a lightweight clock tree before each timing-driven iteration. |
| `-virtual_cts.max_skew_fraction` | float (in [0, 1]) | `0.1` | Maximum clock insertion delay as a fraction of the clock period. |
| `-routability.enabled` | flag |  | Inflate the cells in congested tiles until the target RC metric is met. |
| `-routability.no_use_rudy` | flag |  | Estimate congestion with RUDY; otherwise run the global router (global_route below). |
| `-routability.check_overflow` | float (in [0, 1]) | `0.3` | Overflow below which congestion is checked. |
| `-routability.snapshot_overflow` | float (in [0, 1]) | `0.6` | Overflow at which the placement is saved for a revert. |
| `-routability.max_density` | float (in [0, 1]) | `0.99` | Maximum target density inflation may raise it to. |
| `-routability.target_rc_metric` | float (> 0) | `1.01` | Routing congestion metric to reach; raise it when routability iterations take too long. |
| `-routability.inflation_ratio_coef` | float (> 0) | `2` | Coefficient of the inflation ratio. |
| `-routability.max_inflation_ratio` | float (> 0) | `3` | Maximum inflation of one cell. |
| `-routability.min_congestion_for_inflation` | float (>= 0) | `0.95` | Minimum tile congestion for its cells to be inflated. |
| `-routability.max_inflation_total` | float (> 0) | `0.3` | Ceiling on the area inflation may add, as a fraction of the movable area. |
| `-routability.net_weight_max` | float (> 0) | `1` | Wirelength weight of the nets that put the most wire into congested tiles; 1 disables it. |
| `-routability.congested_nets_percentage` | float (in [0, 100]) | `1` | Share of those nets that is weighted. |
| `-routability.rc_coefficients` | list of 4 float (>= 0) | `{1 1 0 0}` | Coefficients {k1 k2 k3 k4} of the RC metric. |
| `-routability.global_route.no_allow_congestion` | flag |  | Finish with congestion left instead of failing. |
| `-routability.global_route.congestion_iterations` | int (>= 0) | `0` | Rip-up and reroute iterations to remove congestion; CUGR saturates around 5. |
| `-routability.global_route.congestion_report_iter_step` | int (>= 0) | `0` | Iterations between congestion reports; 0 for none. |
| `-routability.global_route.congestion_report_file` | string | `` | Congestion report file for the DRC viewer; empty for none. |
| `-routability.global_route.grid_origin` | list of 2 int | `{0 0}` | Origin {x y} of the routing grid in DBU. |
| `-routability.global_route.critical_nets_percentage` | float (in [0, 100]) | `0` | Share of the nets with the worst slack that get preference during congestion iterations; unset keeps the router's current setting. |
| `-routability.global_route.res_aware_nets_percentage` | float (in [0, 100]) | `none` | Share of the nets routed resistance-aware; needs resistance_aware. Unset keeps the router's default. |
| `-routability.global_route.resistance_aware` | flag |  | Resistance-aware layer assignment and 3D routing (experimental). |
| `-routability.global_route.snapshot_batched_width` | int (>= 0) | `0` | Batches per wave of snapshot-batched routing; 0 for the non-batched behavior (research use). |
| `-routability.global_route.skip_large_fanout_nets` | int (>= 0) | `2147483647` | Nets with more pins than this are not routed. |
| `-routability.global_route.infinite_capacity` | flag |  | Unlimited gcell capacity, for testing. |
| `-routability.global_route.verbose` | flag |  | Report the full routing log. |
| `-routability.global_route.use_cugr` | flag |  | Route with CUGR instead of FastRoute; unset keeps the engine selected earlier (not ready for production). |
| `-routability.global_route.save_guides` | flag |  | Write the routes to the database as guides. |

#### Aliases

The older switch names map onto the switches above: `-timing_driven`
(`-timing.enabled`), `-routability_driven`, `-virtual_cts`,
`-skip_initial_place` (`-initial.max_iter 0`), `-init_density_penalty`,
`-overflow`, `-density uniform` (`-uniform_density`), `-bin_grid_count`
(both directions), `-routability_use_grt` (`-routability.no_use_rudy`),
`-disable_pin_density_adjust`, `-disable_revert_if_diverge`,
`-enable_routing_congestion`, `-skip_io`, `-random_seed`, `-perturb_dist`,
and the `-initial_place_*`, `-routability_*` and `-timing_driven_*` names.

### Placement Configuration

The `set_place_config` command keeps placement settings in the design's block
properties, so they persist when the design is written and read back. Each call
changes only the options it is given. The options below set the slots where
the IO pins are placed: `place_pins` uses them for any switch it is not given,
and `global_placement -place_ios` reads them to place and legalize the pins.
Regions and groups of pins are not part of these settings; define them with
`exclude_io_pin_region` and `set_io_pin_constraint`.

```tcl
set_place_config
    [-io_pin_hor_layers h_layers]
    [-io_pin_ver_layers v_layers]
    [-io_pin_corner_avoidance distance]
    [-io_pin_min_distance min_dist]
    [-io_pin_min_distance_in_tracks]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-io_pin_hor_layers` | The layers to create the metal shapes of pins placed in horizontal tracks. It can be a single layer or a list of layer names. |
| `-io_pin_ver_layers` | The layers to create the metal shapes of pins placed in vertical tracks. It can be a single layer or a list of layer names. |
| `-io_pin_corner_avoidance` | The distance (in microns) from each corner within which pin placement should be avoided. The default value is `1` micron or 15 tracks, whichever is smaller. |
| `-io_pin_min_distance` | The minimum distance between pins on the die boundary. This distance can be in microns (default) or in number of tracks between each pin. The default value is the length of two routing tracks between each pin. |
| `-io_pin_min_distance_in_tracks` | Flag that allows setting the min distance in number of tracks instead of microns. It requires `-io_pin_min_distance`. |

### Reset Placement Configuration

The `reset_place_config` command removes the given settings, or all of them
when no option is given.

```tcl
reset_place_config
    [-io_pin_hor_layers]
    [-io_pin_ver_layers]
    [-io_pin_corner_avoidance]
    [-io_pin_min_distance]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-io_pin_hor_layers` | Remove the horizontal pin layers. |
| `-io_pin_ver_layers` | Remove the vertical pin layers. |
| `-io_pin_corner_avoidance` | Remove the corner avoidance distance. |
| `-io_pin_min_distance` | Remove the minimum distance between pins, and whether it is in tracks. |

### Report Placement Configuration

The `report_place_config` command reports the current settings.

```tcl
report_place_config
```

### Cluster Flops

This command does flop clustering based on parameters.

```tcl
cluster_flops
    [-tray_weight tray_weight]\
    [-timing_weight timing_weight]\
    [-max_split_size max_split_size]\
    [-num_paths num_paths]\
    [-clock_power_weight clock_power_weight]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-tray_weight` | Tray weight, default value is 32.0, type `float`. |
| `-timing_weight` | Timing weight, default value is 0.1, type `float`. |
| `-max_split_size` | Maximum split size, default value is 500 (-1 for no decomposition), type `int`.|
| `-num_paths` | KIV, default value is 0, type `int`. |
| `-clock_power_weight` | Credits clock-tree power saved by banking, promoting higher banking ratios. Models a per-sink clock-tree power equal to this weight times the single-bit flop power; an N-bit tray presents one clock sink instead of N. Default value is 0.0 (legacy cost model, no clock-tree credit), type `float` (must be non-negative). Any non-negative value is accepted, including fractional weights (e.g. 0.5, 1.5); sweep it continuously (typically around 1-3) to reach the desired banking on designs where reduced clock-tree routing is the goal. |


### Placement Clusters

This command defines instances that should be placed as a single cluster.  It is intended for use with small clusters of gates.

```tcl
placement_cluster
    instance_patterns
```

### Debug Mode

The `global_placement_debug` command initiates a debug mode, enabling real-time visualization of the algorithm's progress on the layout. Use the command prior to executing the `global_placement` command, for example on ORFS `flow/scripts/global_place.tcl` script.

```tcl
global_placement_debug
    [-pause pause]
    [-update update]
    [-inst inst]
    [-pin pin]
    [-start_iter start_iter]
    [-start_rudy start_rudy]
    [-rudy_stride rudy_stride]
    [-images_path images_path]
    [-draw_bins]
    [-initial]
    [-generate_images]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-pause` | Number of iterations between pauses during debugging. Allows for visualization of the current state. Allowed values are integers, default is `10`. |
| `-update` | Defines the frequency (in iterations) at which the tool refreshes its layout output to display the latest state during debugging. Allowed values are integers, default is `10`. |
| `-inst` | Targets a specific instance name for debugging focus. The default behavior focuses on no specific instance. |
| `-pin` | Targets a specific IO pin name for debugging focus under `-place_ios`. The default behavior focuses on no specific pin. |
| `-start_iter` | Start debug mode from this iteration. |
| `-start_rudy` | Start RUDY debug from this iteration. |
| `-rudy_stride` | Stride for RUDY debug iterations. Allowed values are integers, default is `1`. |
| `-images_path` | Path for saving generated images. |
| `-draw_bins` | Activates visualization of placement bins, showcasing their density (indicated by the shade of white) and the direction of forces acting on them (depicted in red). The default setting is disabled. |
| `-initial` | Pauses the debug process during the initial placement phase. The default setting is disabled. |
| `-generate_images` | Generates a GIF animation showing the placement progression and a GIF composed of images right before each routability revert. Also saves snapshot images at the end of each routability and timing-driven iteration, including heatmaps. |

Example: `global_placement_debug -pause 100 -update 1 -initial -draw_bins -inst _614_`
This command configures the debugger to pause every 100 iterations, with layout updates occurring every iteration. It enables initial placement stage visualization, bin drawing, and specifically highlights instance 614.

Debug mode requires the GUI. With ORFS, one way to arrange for showing the GUI when running the global placement step is by using `make global_place_issue` and then editing the created run-me.sh file to include the `-gui` flag when calling openroad.

### Estimate Target Density

This command estimates the target density needed to reach a given overflow for the current placement, without running global placement or otherwise modifying the design.

The returned density is always between the design's uniform target density (the same value reported by `get_global_placement_uniform_density`) and `1.0`. A tighter (lower) `-overflow` value can only ask for a density closer to `1.0`, never lower than the uniform density.

```tcl
estimate_target_density
    [-bin_grid_count grid_count]\
    [-overflow overflow]\
    [-pad_left pad_left]\
    [-pad_right pad_right]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-bin_grid_count` | Set bin grid's counts. The internal heuristic defines the default value. Allowed values are integers `[64,128,256,512,...]`. |
| `-overflow` | Set target overflow used to estimate the density. The default value is `0.1`. Allowed values are floats `[0, 1]`. |
| `-pad_left` | Set left padding in terms of number of sites. The default value is `0`, and the allowed values are integers `[0, MAX_INT]` |
| `-pad_right` | Set right padding in terms of number of sites. The default value is `0`, and the allowed values are integers `[0, MAX_INT]` |

## Useful Developer Commands

If you are a developer, you might find these useful. More details can be found in the [source file](./src/replace.cpp) or the [swig file](./src/replace.i).

```
# adds padding and gets global placement uniform target density
get_global_placement_uniform_density -pad_left -pad_right 
```
Example scripts demonstrating how to run `gpl` on a sample design on `core01` as follows:

```shell
./test/core01.tcl
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following script:

```shell
./test/regression
```

## Limitations

## Using the Python interface to gpl

This API tries to stay close to the API defined in `C++` class `Replace`
that is located [here](include/gpl/Replace.h). The options are the
`gpl.PlaceOptions` class, the same struct the Tcl `-options` dict fills:
nested dicts or keyword arguments, attribute access, and the same checks and
messages as Tcl.

```python
from openroad import Design, Tech
import gpl

tech = Tech()
tech.readLef(...)
design = Design(tech)
design.readDef(...)

options = gpl.PlaceOptions(
    density=0.72,
    timing=dict(enabled=True, nets_percentage=15, repair_design=dict(slew_margin=5)),
)
options.initial.max_iter = 0            # attributes, nested
options.update({"nesterov.init_density_penalty": 0.01})  # dotted keys
print(options.to_dict(only_changed=True))
print(gpl.PlaceOptions.describe())     # help, as describe_options
design.getReplace().doPlace(threads=1, options=options)
```

## References

-   C.-K. Cheng, A. B. Kahng, I. Kang and L. Wang, "RePlAce: Advancing
    Solution Quality and Routability Validation in Global Placement", IEEE
    Transactions on Computer-Aided Design of Integrated Circuits and Systems,
    38(9) (2019), pp. 1717-1730. [(.pdf)](https://vlsicad.ucsd.edu/Publications/Journals/j126.pdf)
-   J. Lu, P. Chen, C.-C. Chang, L. Sha, D. J.-H. Huang, C.-C. Teng and
    C.-K. Cheng, "ePlace: Electrostatics based Placement using Fast Fourier
    Transform and Nesterov's Method", ACM TODAES 20(2) (2015), article 17. [(.pdf)](https://cseweb.ucsd.edu/~jlu/papers/eplace-todaes14/paper.pdf)
-   J. Lu, H. Zhuang, P. Chen, H. Chang, C.-C. Chang, Y.-C. Wong, L. Sha,
    D. J.-H. Huang, Y. Luo, C.-C. Teng and C.-K. Cheng, "ePlace-MS:
    Electrostatics based Placement for Mixed-Size Circuits", IEEE TCAD 34(5)
    (2015), pp. 685-698. [(.pdf)](https://cseweb.ucsd.edu/~jlu/papers/eplace-ms-tcad14/paper.pdf)
-   A. B. Kahng, J. Li and L. Wang,   
    "Improved Flop Tray-Based Design Implementation for Power Reduction",   
    IEEE/ACM ICCAD, 2016, pp. 20:1-20:8.    
-   A. B. Kahng, S. Kundu, S. Thumathy,    
    "Scalable Flip-Flop Clustering Using Divide and Conquer For Capacitated K-Means".   
    ACM GLSVLSI, 2024, pp. 177-184.[(.pdf)](https://vlsicad.ucsd.edu/Publications/Conferences/409/c409.pdf)      
-   The timing-driven mode has been implemented by
    Mingyu Woo (only available in [legacy repo in standalone
    branch](https://github.com/The-OpenROAD-Project/RePlAce/tree/standalone).)
-   The routability-driven mode has been implemented by Mingyu Woo.
-   Timing-driven mode re-implementation is ongoing with the current
    clean-code structure.
-   RUDY: Spindler, Peter, and Frank M. Johannes. "Fast and accurate routing 
    demand estimation for efficient routability-driven placement. In 2007 
    Design, Automation & Test in Europe Conference & Exhibition." (2007): 1-6.
    [(.pdf)](https://past.date-conference.com/proceedings-archive/2007/DATE07/PDFFILES/08.7_1.PDF)

 ## Authors

-   Authors/maintainer since Jan 2020: Mingyu Woo (Ph.D. Advisor:
    Andrew. B. Kahng)
-   Original open-sourcing of RePlAce: August 2018, by Ilgweon Kang
    (Ph.D. Advisor: Chung-Kuan Cheng), Lutong Wang (Ph.D. Advisor: Andrew
    B. Kahng), and Mingyu Woo (Ph.D. Advisor: Andrew B. Kahng).
-   Also thanks to Dr. Jingwei Lu for open-sourcing the previous
    ePlace-MS/ePlace project code.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
