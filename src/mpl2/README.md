# Hierarchical Macro Placement

A hierarchical automatic macro placer for large-scale complex IP blocks, "Hier-RTLMP".
This tool builds on the existing RTLMP (`mpl`) framework, adopting a multilevel physical 
planning approach that exploits the hierarchy and data flow inherent in the design RTL.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Hier-RTLMP algorithm

```tcl
rtl_macro_placer 
    [-halo_width halo_width]
    [-min_num_macro min_num_macro]
    [-max_num_inst max_num_inst]  
    [-min_num_inst min_num_inst] 
    [-tolerance tolerance]     
    [-max_num_level max_num_level] 
    [-coarsening_ratio coarsening_ratio]
    [-num_bundled_ios num_bundled_ios]
    [-large_net_threshold large_net_threshold]
    [-signature_net_threshold signature_net_threshold]
    [-halo_width halo_width] 
    [-fence_lx fence_lx] 
    [-fence_ly fence_ly]
    [-fence_ux fence_ux]
    [-fence_uy fence_uy]
    [-area_weight area_weight] 
    [-outline_weight outline_weight] 
    [-wirelength_weight wirelength_weight]
    [-guidance_weight guidance_weight]
    [-fence_weight fence_weight] 
    [-boundary_weight boundary_weight]
    [-notch_weight notch_weight]
    [-macro_blockage_weight macro_blockage_weight]
    [-target_util target_util]
    [-target_dead_space target_dead_space]
    [-min_ar min_ar]
    [-snap_layer snap_layer]
    [-bus_planning_flag bus_planning_flag]
    [-report_directory report_directory]
    [-write_macro_placement file_name]
```

#### Generic Parameters

| Switch Name | Description |
| ----- | ----- |
| `-max_num_macro`, `-min_num_macro` | Maximum/minimum number of macros in a cluster. The default value is `0` for both, and the allowed values are integers `[0, MAX_INT]`. |
| `-max_num_inst`, `-min_num_inst` | Maximum/minimum number of standard cells in a cluster. The default value is `0` for both, and the allowed values are integers `[0, MAX_INT]`. |
| `-tolerance` | Add a margin to the minimum and maximum number of macros/std cells in a cluster. For min, we multiply by (1 - `tol`), and for the max (1 + `tol`). This is to improve the robustness of hierarchical clustering. The allowed values are floats `[0, 1)`, and the default value is `0.1`. |
| `-max_num_level` | Maximum depth of physical hierarchical tree. The default value is `2`, and the allowed values are integers `[0, MAX_INT]`. |
| `-coarsening_ratio` | The larger the coarsening_ratio, the faster the convergence process. The allowed values are floats, and the default value is `10.0`. |
| `-num_bundled_ios` | Specifies the number of bundled pins for the left, right, top, and bottom boundaries. The default value is `3`, and the allowed values are integers `[0, MAX_INT]`. |
| `-large_net_threshold` | Ignore nets with many connections during clustering, such as global nets. The default value is `50`, and the allowed values are integers `[0, MAX_INT]`. |
| `-signature_net_threshold` | Minimum number of connections between two clusters to be identified as connected. The default value is `50`, and the allowed values are integers `[0, MAX_INT]`. |
| `-halo_width` | iHorizontal/vertical halo around macros (microns). The allowed values are floats, and the default value is `0.0`. |
| `-fence_lx`, `-fence_ly`, `-fence_ux`, `-fence_uy` | Defines the global fence bounding box coordinates. The default values are the core area coordinates). |
| `-target_util` | Specifies the target utilization of `MixedCluster` and has higher priority than target_dead_space. The allowed values are floats, and the default value is `0.25`. |
| `-target_dead_space` | Specifies the target dead space percentage, which influences the utilization of `StandardCellCluster`. The allowed values are floats, and the default value is `0.05`. |
| `-min_ar` | Specifies the minimum aspect ratio $a$, or the ratio of its width to height of a `StandardCellCluster` from $[a, \frac{1}{a}]$. The allowed values are floats, and the default value is `0.33`. |
| `-snap_layer` | Snap macro origins to this routing layer track. The default value is 4, and the allowed values are integers `[1, MAX_LAYER]`). |
| `-bus_planning_flag` | Flag to enable bus planning. The recommendation is to turn on bus planning for SKY130 and off for NanGate45/ASAP7. The default value is disabled. |
| `-report_directory` | Save reports to this directory. |
| `-write_macro_placement` | Generates a file with the macro placement in the format of multiple calls for the `place_macro` command. |


#### Simulated Annealing Weight parameters

Do note that while action probabilities are normalized to 1.0, the weights are not necessarily normalized. 

| Switch Name | Description | 
| ----- | ----- |
| `-area_weight` | Weight for the area of current floorplan.  The allowed values are floats, and the default value is `0.1`. |
| `-outline_weight` | Weight for violating the fixed outline constraint, meaning that all clusters should be placed within the shape of their parent cluster.  The allowed values are floats, and the default value is `100.0`. |
| `-wirelength_weight` | Weight for half-perimeter wirelength.  The allowed values are floats, and the default value is `100.0`. |
| `-guidance_weight` | Weight for guidance cost or clusters being placed near specified regions if users provide such constraints.  The allowed values are floats, and the default value is `10.0`. |
| `-fence_weight` | Weight for fence cost, or how far the macro is from zero fence violation.  The allowed values are floats, and the default value is `10.0`. |
| `-boundary_weight` | Weight for the boundary, or how far the hard macro clusters are from boundaries. Note that mixed macro clusters are not pushed, thus not considered in this cost.  The allowed values are floats, and the default value is `50.0`. |
| `-notch_weight` | Weight for the notch, or the existence of dead space that cannot be used for placement & routing. Note that this cost applies only to hard macro clusters.  The allowed values are floats, and the default value is `10.0`. |
| `-macro_blockage_weight` | Weight for macro blockage, or the overlapping instances of the macro.  The allowed values are floats, and the default value is `10.0`. |

### Write Macro Placement

Command to write a file with the macro placement in the format of multiple calls for the `place_macro` command:

```tcl
write_macro_placement file_name
```

### Place Macro

Command for placement of one specific macro.

```tcl
place_macro
    -macro_name macro_name
    -location {x y}
    [-orientation orientation]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-macro_name` | The name of a macro of the design. |
| `-location` | The lower left corner of the macro in microns. |
| `-orientation` | The orientation according to odb. If nothing is specified, defaults to `R0`.  We only allow `R0`, `MY`, `MX` and `R180`.  |

## Example scripts

Example of a script demonstrating how to run `mpl2` on a sample design of `bp_fe_top` as follows:

```shell
./test/bp_fe_top.tcl
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following script:

```shell
./test/regression
```

## Limitations

## References
1. A. B. Kahng, R. Varadarajan and Z. Wang, 
"RTL-MP: Toward Practical, Human-Quality Chip Planning and Macro Placement",
[(.pdf)](https://vlsicad.ucsd.edu/Publications/Conferences/389/c389.pdf), Proc. ACM/IEEE Intl. Symp. on Physical Design, 2022, pp. 3-11.
1. A. B. Kahng, R. Varadarajan and Z. Wang,
"Hier-RTLMP: A hierarchical automatic macro placer for large-scale complex IP blocks.",
[(.pdf)](https://arxiv.org/pdf/2304.11761.pdf), arXiv preprint arXiv:2304.11761, 2023.

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+hier-rtlmp+OR+hier+OR+mpl2) about this tool.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
