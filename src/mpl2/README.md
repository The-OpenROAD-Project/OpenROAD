# Macro Placement

A Hierarchical Automatic Macro Placer for Large-scale Complex IP Blocks, "Hier-RTLMP".
This tool builds on the existing RTLMP (*mpl*) framework, adopting a multilevel physical 
planning approach that exploits the hierarchy and dataflow inherent in the design RTL.

## Commands
```
rtl_macro_placer [-halo_width   halo_width]
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
                 [-pin_access_th    pin_access_th]
                 [-target_util  target_util]
                 [-target_dead_space    target_dead_space]
                 [-min_ar   min_ar]
                 [-snap_layer   snap_layer]
                 [-bus_planning_flag    bus_planning_flag]
                 [-report_directory report_directory]

```
### Generic Parameters 
-   `-max_num_macro`, `min_num_macro` maximum/minimum number of macros in a cluster.
-   `-max_num_inst`, `min_num_inst` maximum/minimum number of standard cells in a cluster.
-   `-tolerance` a factor of $(1 + tol)$ is multiplied to the max, $(1 - tol)$ to the min number of 
macros/std cells in a cluster respectively. This is to improve robustness of hierarchical clustering.
(default 0.1)
-   `-max_num_level` maximum depth of physical hierarchical tree.
-   `-coarsening_ratio` larger the coarsening_ratio, the faster the convergence process. (default 5.0)
-   `-num_bundled_ios` specifies number of bundled pins for left, right, top, bottom boundary. (default 3)
-   `large_net_threshold` ignore nets with many connections during clustering, such as global nets. (default 100)
-   `signature_net_threshold` minimum number of connections between two clusters for them to be identified as connected. (default 20)
-   `-halo_width` horizontal/vertical halo around macros (microns).
-   `-fence_lx`, `fence_ly`, `fence_ux`, `fence_uy` defines the global fence bounding box coordinates. (defaults to core area coordinates).
-   `target_util` target utilisation of MixedCluster, and has higher priority than target_dead_space (default 0.25)
-   `target_dead_space` target deadspace percentage, which influences the utilisation of StandardCellCluster(default 0.05)
-   `min_ar` defines the minimum aspect ratio $a$, or the ratio of its width to height of a StandardCellCluster from $[a, \frac{1}{a}]$ (default 0.3)
-   `snap_layer` snap macro origins to this routing layer track. (default 4)
-   `bus_planning_flag` flag to enable bus planning, recommendation is to turn off bus planning for better results.  (default false)
-   `report_directory` save reports to this directory. 



### Simulated Annealing Weight parameters
Do note that while action probabilities are normalized to 1.0, the weights are not necessarily normalized. 
-   `area_weight` weight for area of current floorplan. (default 0.1)
-   `outline_weight` weight for violating the fixed outline constraint, meaning that all clusters should be placed within the shape of its parent cluster. (default 1.0)
-   `wirelength_weight` weight for half-perimeter wirelength. (default 1.0)
-   `guidance_weight` weight for guidance cost, or clusters being placed near specified regions if users provide such constraints. (default 10.0)
-   `fence_weight` weight for fence cost, or how far the macro is from zero fence violation. (default 10.0)
-   `boundary_weight` weight for boundary, or how far the hard macro clusters are from boundaries. Note that mixed macro clusters are not pushed, thus not considered in this cost. (default 5.0)
-   `notch_weight` weight for notch, or the existence of dead space that cannot be used for placement & routing. Note that this cost applies only to hard macro clusters. (default 1.0)
-   `macro_blockage_weight` weight for macro blockage, or the overlapping instances of macro. (default 1.0)

## References
A. B. Kahng, R. Varadarajan and Z. Wang, 
"RTL-MP: Toward Practical, Human-Quality Chip Planning and Macro Placement",
[(.pdf)](https://vlsicad.ucsd.edu/Publications/Conferences/389/c389.pdf), Proc. ACM/IEEE Intl. Symp. on Physical Design, 2022, pp. 3-11.
A. B. Kahng, R. Varadarajan and Z. Wang,
"Hier-RTLMP: A hierarchical automatic macro placer for large-scale complex IP blocks.",
[(.pdf)](https://arxiv.org/pdf/2304.11761.pdf), arXiv preprint arXiv:2304.11761, 2023.


## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
