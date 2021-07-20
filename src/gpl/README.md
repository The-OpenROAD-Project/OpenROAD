# RePlAce

RePlAce: Advancing Solution Quality and Routability Validation in Global Placement

## Commands

```
global_placement
    [-timing_driven]
    [-routability_driven]
    [-skip_initial_place]
    [-disable_timing_driven]
    [-disable_routability_driven]
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
    [-pad_left pad_left]
    [-pad_right pad_right]
    [-verbose_level level]
```

- **timing_driven**: Enable timing-driven mode
* __skip_initial_place__ : Skip the initial placement (BiCGSTAB solving) before Nesterov placement. IP improves HPWL by ~5% on large designs. Equal to '-initial_place_max_iter 0'
* __incremental__ : Enable the incremental global placement. Users would need to tune other parameters (e.g. init_density_penalty) with pre-placed solutions.
- **grid_count**: [64,128,256,512,..., int]. Default: Defined by internal algorithm.

### Tuning Parameters

| Parameter                    | Description                                                                          | Default                        | Allowed values             |
|------------------------------|--------------------------------------------------------------------------------------|--------------------------------|----------------------------|
| __bin_grid_count__           | Set bin grid's counts.                                                               | Defined by internal algorithm. | [ 64,128,256,512,..., int] |
| __density__                  | Set target density.                                                                  | 0.70                           | [ 0-1, float]              |
| __init_density_penalty__     | Set initial density penalty.                                                         | 8e-5                           | [ 1e-6 - 1e6, float]       |
| __init_wire_length_coef__    | Set initial wirelength coefficient.                                                  | 0.25                           | [ unlimited, float]        |
| __min_phi_coef__             | Set pcof_min(µ_k Lower Bound).                                                       | 0.95                           | [ 0.95-1.05, float]        |
| __max_phi_coef__             | Set pcof_max(µ_k Upper Bound).                                                       | 1.05                           | [ 1.00-1.20, float]        |
| __overflow__                 | Set target overflow for termination condition.                                       | 0.1                            | [ 0-1, float]              |
| __initial_place_max_iter__   | Set maximum iterations in initial place.                                             | 20                             | [ 0-, int]                 |
| __initial_place_max_fanout__ | Set net escape condition in initial place when 'fanout >= initial_place_max_fanout'. | 200                            | [ 1-, int]                 |
| __verbose_level__            | Set verbose level for RePlAce.                                                       | 1                              | [ 0-10, int]               |


`-timing_driven` does a virtual 'repair_design' to find slacks and
weight nets with low slack.  Use the `set_wire_rc` command to set
resistance and capacitance of estimated wires used for timing.

## Features
- Analytic and nonlinear placement algorithm. Solves electrostatic force equations using Nesterov's method. ([link](https://cseweb.ucsd.edu/~jlu/papers/eplace-todaes14/paper.pdf))
- Verified with various commercial technologies using OpenDB (7/14/16/28/45/55/65nm).
- Verified deterministic solution generation with various compilers and OS.
  * Compiler: gcc4.8-9.1/clang-7-9/apple-clang-11
  * OS: Ubuntu 16.04-18.04 / CentOS 6-8 / OSX
- Cleanly rewritten as C++11.
- Supports Mixed-size placement mode.
- Supports fast image drawing modes with CImg library.

| <img src="./doc/image/adaptec2.inf.gif" width=350px> | <img src="./doc/image/coyote_movie.gif" width=400px> |
|:--:|:--:|
| *Visualized examples from ISPD 2006 contest; adaptec2.inf* |*Real-world Design: Coyote (TSMC16 7.5T)* |

 ## Authors
- Authors/maintainer since Jan 2020: Mingyu Woo (Ph.D. Advisor: Andrew. B. Kahng)
- Original open-sourcing of RePlAce: August 2018, by Ilgweon Kang (Ph.D. Advisor: Chung-Kuan Cheng), Lutong Wang (Ph.D. Advisor: Andrew B. Kahng), and Mingyu Woo (Ph.D. Advisor: Andrew B. Kahng).
- Also thanks to Dr. Jingwei Lu for open-sourcing the previous ePlace-MS/ePlace project from Dr. Jingwei Lu.

- Paper reference:
  - C.-K. Cheng, A. B. Kahng, I. Kang and L. Wang, "RePlAce: Advancing Solution Quality and Routability Validation in Global Placement", IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems, 38(9) (2019), pp. 1717-1730.
  - J. Lu, P. Chen, C.-C. Chang, L. Sha, D. J.-H. Huang, C.-C. Teng and C.-K. Cheng, "ePlace: Electrostatics based Placement using Fast Fourier Transform and Nesterov's Method", ACM TODAES 20(2) (2015), article 17.
  - J. Lu, H. Zhuang, P. Chen, H. Chang, C.-C. Chang, Y.-C. Wong, L. Sha, D. J.-H. Huang, Y. Luo, C.-C. Teng and C.-K. Cheng, "ePlace-MS: Electrostatics based Placement for Mixed-Size Circuits", IEEE TCAD 34(5) (2015), pp. 685-698.

- The timing-driven mode has been implemented by Mingyu Woo (only available in [legacy repo in standalone branch](https://github.com/The-OpenROAD-Project/RePlAce/tree/standalone).)
- The routability-driven mode has been implemented by Mingyu Woo.
- Timing-driven mode re-implementation is ongoing with the current clean-code structure.

## License
* BSD-3-clause License [[Link]](./LICENSE.md)
