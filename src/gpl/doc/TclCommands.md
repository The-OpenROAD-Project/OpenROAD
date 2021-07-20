# OpenROAD Tcl Usage (global_placement)

```
global_placement
    [-skip_initial_place]
    [-incremental]
    [-bin_grid_count grid_count]
    [-density density]
    [-init_density_penalty init_density_penalty]
    [-init_wirelength_coef init_wirelength_coef]
    [-min_phi_coef min_phi_coef]
    [-max_phi_coef max_phi_coef]
    [-overflow overflow]
    [-initial_place_max_iter max_iter]
    [-initial_place_max_fanout max_fanout]
    [-verbose_level verbose_level]

```

## Flow Control
* __skip_initial_place__ : Skip the initial placement (BiCGSTAB solving) before Nesterov placement. IP improves HPWL by ~5% on large designs.
* __incremental__ : Enable the incremental global placement. Users would need to tune other parameters (e.g. init_density_penalty) with the pre-placed solutions.

## Tuning Parameters
* __bin_grid_count__ : Set bin grid's count manually. Default: Defined by internal algorithm. [64,128,256,512,..., int]
* __density__ : Set target density. Default: 0.70 [0-1, float]
* __init_density_penalty__ : Set initial density penalty. Default : 8e-5 [1e-6 - 1e6, float]
* __min_phi_coef__ : Set pcof_min(µ_k Lower Bound). Default: 0.95 [0.95-1.05, float]
* __max_phi_coef__ : Set pcof_max(µ_k Upper Bound). Default: 1.05 [1.00-1.20, float]
* __overflow__ : Set target overflow for termination condition. Default: 0.1 [0-1, float]

## Other Options
* __verbose_level__ [0-10, int] : Set verbose level for RePlAce. Default: 1

Note that all of the TCL commands are defined in the [../src/replace.tcl](https://github.com/The-OpenROAD-Project/OpenROAD/blob/master/src/replace/src/replace.tcl) and [../src/replace.i](https://github.com/The-OpenROAD-Project/OpenROAD/blob/master/src/replace/src/replace.i).
