# Hierarchical Macro Placement

A hierarchical automatic macro placer for large-scale complex IP blocks, "Hier-RTLMP".
This tool builds on the existing RTLMP (*mpl*) framework, adopting a multilevel physical 
planning approach that exploits the hierarchy and dataflow inherent in the design RTL.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Hier-RTLMP algorithm

```tcl
rtl_macro_placer 
    [-halo_width   halo_width]
    [-min_num_macro    min_num_macro]
    [-max_num_inst max_num_inst]  
    [-min_num_inst min_num_inst] 
    [-tolerance    tolerance]     
    [-max_num_level    max_num_level] 
    [-coarsening_ratio coarsening_ratio]
    [-num_bundled_ios  num_bundled_ios]
    [-large_net_threshold  large_net_threshold]
    [-signature_net_threshold  signature_net_threshold]
    [-halo_width   halo_width] 
    [-fence_lx fence_lx] 
    [-fence_ly fence_ly]
    [-fence_ux fence_ux]
    [-fence_uy fence_uy]
    [-area_weight  area_weight] 
    [-outline_weight   outline_weight] 
    [-wirelength_weight    wirelength_weight]
    [-guidance_weight  guidance_weight]
    [-fence_weight fence_weight] 
    [-boundary_weight  boundary_weight]
    [-notch_weight notch_weight]
    [-macro_blockage_weight    macro_blockage_weight]
    [-target_util  target_util]
    [-target_dead_space    target_dead_space]
    [-min_ar   min_ar]
    [-snap_layer   snap_layer]
    [-bus_planning_flag    bus_planning_flag]
    [-report_directory report_directory]
```

#### Generic Parameters

| Switch Name | Description |
| ----- | ----- |
| `-max_num_macro`, `min_num_macro` | Maximum/minimum number of macros in a cluster. (default 0, 0). |
| `-max_num_inst`, `min_num_inst` | Maximum/minimum number of standard cells in a cluster. (default 0, 0). |
| `-tolerance` | A factor of $(1 + tol)$ is multiplied to the max, $(1 - tol)$ to the min number of macros/std cells in a cluster respectively. This is to improve robustness of hierarchical clustering. (default 0.1). |
| `-max_num_level` | Maximum depth of physical hierarchical tree. (default 2). |
| `-coarsening_ratio` | The larger the coarsening_ratio, the faster the convergence process. (default 10.0). | 
| `-num_bundled_ios` | Specifies number of bundled pins for left, right, top, bottom boundary. (default 3). |
| `large_net_threshold` | Ignore nets with many connections during clustering, such as global nets. (default 50). |
| `signature_net_threshold` | Minimum number of connections between two clusters for them to be identified as connected. (default 50). |
| `-halo_width` | Horizontal/vertical halo around macros (microns). (default 0.0). |
| `-fence_lx`, `fence_ly`, `fence_ux`, `fence_uy` | Defines the global fence bounding box coordinates. (defaults to core area coordinates). |
| `target_util` | Specifies the target utilisation of MixedCluster, and has higher priority than target_dead_space (default 0.25). |
| `target_dead_space` | Specifies the target deadspace percentage, which influences the utilisation of StandardCellCluster(default 0.05). |
| `min_ar` | Specifies the minimum aspect ratio $a$, or the ratio of its width to height of a StandardCellCluster from $[a, \frac{1}{a}]$ (default 0.33). |
| `snap_layer` | Snap macro origins to this routing layer track. (default -1). | 
| `bus_planning_flag` | Flag to enable bus planning, recommendation is to turn on bus planning for SKY130, off for NanGate45/ASAP7.  (default false). |
| `report_directory` | Save reports to this directory. |


#### Simulated Annealing Weight parameters

Do note that while action probabilities are normalized to 1.0, the weights are not necessarily normalized. 

| Switch Name | Description | 
| ----- | ----- |
| `area_weight` | Weight for the area of current floorplan. (default 0.1). |
| `outline_weight` | Weight for violating the fixed outline constraint, meaning that all clusters should be placed within the shape of its parent cluster. (default 100.0). |
| `wirelength_weight` | Weight for half-perimeter wirelength. (default 100.0). |
| `guidance_weight` | Weight for guidance cost, or clusters being placed near specified regions if users provide such constraints. (default 10.0). |
| `fence_weight` | Weight for fence cost, or how far the macro is from zero fence violation. (default 10.0). |
| `boundary_weight` | Weight for boundary, or how far the hard macro clusters are from boundaries. Note that mixed macro clusters are not pushed, thus not considered in this cost. (default 50.0). |
| `notch_weight` | Weight for notch, or the existence of dead space that cannot be used for placement & routing. Note that this cost applies only to hard macro clusters. (default 10.0). |
| `macro_blockage_weight` | Weight for macro blockage, or the overlapping instances of macro. (default 10.0). |

## Example scripts

Examples scripts demonstrating how to run Hier-RTLMP on a sample design of `bp_fe_top` as follows:

```shell
./test/bp_fe_top.tcl
```

## Regression tests

There are a set of regression tests in `/test`. For more information, refer to this [section](../../README.md#regression-tests).

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
