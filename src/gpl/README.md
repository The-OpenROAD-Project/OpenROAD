# RePlAce

RePlAce: Advancing Solution Quality and Routability Validation in Global Placement

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

```
global_placement
    [-timing_driven]
    [-routability_driven]
    [-skip_initial_place]
    [-incremental]
    [-bin_grid_count grid_count]
    [-density target_density]
    [-init_density_penalty init_density_penalty]
    [-init_wirelength_coef init_wirelength_coef]
    [-min_phi_coef min_phi_conef]
    [-max_phi_coef max_phi_coef]
    [-overflow overflow]
    [-initial_place_max_iter initial_place_max_iter]
    [-initial_place_max_fanout initial_place_max_fanout]
    [-routability_check_overflow routability_check_overflow]
    [-routability_max_density routability_max_density]
    [-routability_max_bloat_iter routability_max_bloat_iter]
    [-routability_max_inflation_iter routability_max_inflation_iter]
    [-routability_target_rc_metric routability_target_rc_metric]
    [-routability_inflation_ratio_coef routability_inflation_ratio_coef]
    [-routability_pitch_scale routability_pitch_scale]
    [-routability_max_inflation_ratio routability_max_inflation_ratio]
    [-routability_rc_coefficients routability_rc_coefficients]
    [-timing_driven_net_reweight_overflow]
    [-timing_driven_net_weight_max]
    [-timing_driven_nets_percentage]
    [-pad_left pad_left]
    [-pad_right pad_right]
    [-verbose_level level]
    [-force_cpu]
```

### Tuning Parameters

- `-timing_driven`: Enable timing-driven mode
- `-routability_driven`: Enable routability-driven mode
- `-skip_initial_place` : Skip the initial placement (BiCGSTAB solving) before Nesterov placement. IP improves HPWL by ~5% on large designs. Equal to '-initial_place_max_iter 0'
- `-incremental` : Enable the incremental global placement. Users would need to tune other parameters (e.g., init_density_penalty) with pre-placed solutions.
- `-bin_grid_count`: set bin grid's counts. Default value is defined by internal heuristic. Allowed values are  `[64,128,256,512,..., int]`.
- `-density`: set target density. Default value is 0.70. Allowed values are `[0-1, float]`.
- `-init_density_penalty`: set initial density penalty. Default value is 8e-5. Allowed values are `[1e-6 - 1e6, float]`.
- `-init_wirelength_coef`: set initial wirelength coefficient. Default value is 0.25. Allowed values are `[unlimited, float]`.
- `-min_phi_coef`: set `pcof_min(µ_k Lower Bound)`. Default value is 0.95. Allowed values are `[0.95-1.05, float]`.
- `-max_phi_coef`: set `pcof_max(µ_k Upper Bound)`. Default value is 1.05. Allowed values are `[1.00-1.20, float]`.
- `-overflow`: set target overflow for termination condition. Default value is 0.1. Allowed values are `[0-1, float]`.
- `-initial_place_max_iter`: set maximum iterations in initial place. Default value is 20. Allowed values are `[0-MAX_INT, int]`.
- `-initial_place_max_fanout`: set net escape condition in initial place when 'fanout >= initial_place_max_fanout'. Default value is 200. Allowed values are `[1-MAX_INT, int]`.
- `-timing_driven_net_reweight_overflow`: set overflow threshold for timing-driven net reweighting. Allowed values are `tcl list of [0-100, int]`.
- `-timing_driven_net_weight_max`: Set the multiplier for the most timing critical nets. Default value is 1.9.
- `-timing_driven_nets_percentage`: Set the percentage of nets that are reweighted in timing-driven mode. Default value is 10. Allowed values are `[0-100, float]`
- `-verbose_level`: set verbose level for RePlAce. Default value is 1. Allowed values are `[0-5, int]`.
- `-force_cpu`: Force to use the CPU solver even if the GPU is available.


`-timing_driven` does a virtual `repair_design` to find slacks and
weight nets with low slack. It adjusts the worst slacks (10% by default,
modified with -timing_driven_nets_percentage) using a multiplier (1.9 by
default, modified with `-timing_driven_net_weight_max`). The multiplier
is scaled from the full value for the worst slack, to 1.0 at the
timing_driven_nets_percentage point. Use the `set_wire_rc` command to set
resistance and capacitance of estimated wires used for timing.

## Example scripts

## Regression tests

## Limitations

## Using the Python interface to gpl

This api tries to stay close to the api defined in C++ class `Replace`
that is located in gpl/include/gpl/Replace.h

When initializing a design, a sequence of Python commands might look like
the following:

    from openroad import Design, Tech
    tech = Tech()
    tech.readLef(...)
    design = Design(tech)
    design.readDef(...)
    gpl = design.getReplace()
    
Here is an example of some options / configurations to the global placer.
(See Replace.h for a complete list)

    gpl.setInitialPlaceMaxIter(iter)
    gpl.setSkipIoMode(skip_io)
    gpl.setTimingDrivenMode(timing_driven)
    gpl.setTimingNetWeightMax(weight)

There are some useful Python functions located in the file
grt/test/grt_aux.py but these are not considered a part of the (final)
api and they may change.

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+replace+in%3Atitle)
about this tool.

## External references

-   C.-K. Cheng, A. B. Kahng, I. Kang and L. Wang, "RePlAce: Advancing
    Solution Quality and Routability Validation in Global Placement", IEEE
    Transactions on Computer-Aided Design of Integrated Circuits and Systems,
    38(9) (2019), pp. 1717-1730.
-   J. Lu, P. Chen, C.-C. Chang, L. Sha, D. J.-H. Huang, C.-C. Teng and
    C.-K. Cheng, "ePlace: Electrostatics based Placement using Fast Fourier
    Transform and Nesterov's Method", ACM TODAES 20(2) (2015), article 17.
-   J. Lu, H. Zhuang, P. Chen, H. Chang, C.-C. Chang, Y.-C. Wong, L. Sha,
    D. J.-H. Huang, Y. Luo, C.-C. Teng and C.-K. Cheng, "ePlace-MS:
    Electrostatics based Placement for Mixed-Size Circuits", IEEE TCAD 34(5)
    (2015), pp. 685-698.

-   The timing-driven mode has been implemented by
    Mingyu Woo (only available in [legacy repo in standalone
    branch](https://github.com/The-OpenROAD-Project/RePlAce/tree/standalone).)
-   The routability-driven mode has been implemented by Mingyu Woo.
-   Timing-driven mode re-implementation is ongoing with the current
    clean-code structure.

 ## Authors

-   Authors/maintainer since Jan 2020: Mingyu Woo (Ph.D. Advisor:
    Andrew. B. Kahng)
-   Original open-sourcing of RePlAce: August 2018, by Ilgweon Kang
    (Ph.D. Advisor: Chung-Kuan Cheng), Lutong Wang (Ph.D. Advisor: Andrew
    B. Kahng), and Mingyu Woo (Ph.D. Advisor: Andrew B. Kahng).
-   Also thanks to Dr. Jingwei Lu for open-sourcing the previous
    ePlace-MS/ePlace project code.

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
