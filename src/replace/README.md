# RePlAce
RePlAce: Advancing Solution Quality and Routability Validation in Global Placement

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

## How to Download and Build?
- If you want to use this as part of the OpenROAD project you should build it and use it from inside the integrated [OpenROAD app](https://github.com/The-OpenROAD-Project/OpenROAD). The standalone version is available as a legacy code in [standalone branch](https://github.com/The-OpenROAD-Project/RePlAce/tree/standalone).
- For OpenROAD-flow users, manuals for released binaries are available in readthedocs. [(Getting-Started)](https://openroad.readthedocs.io/en/latest/user/getting-started.html)
- For developers, manuals for building a binary is available in OpenROAD app repo. [(OpenROAD app)](https://github.com/The-OpenROAD-Project/OpenROAD) 
- Note that RePlAce is a submodule of OpenROAD repo, and take a place as the **"global_placement"** command. 


## OpenROAD Tcl Usage (global_placement)

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

### Flow Control
* __skip_initial_place__ : Skip the initial placement (BiCGSTAB solving) before Nesterov placement. IP improves HPWL by ~5% on large designs. Equal to '-initial_place_max_iter 0'
* __incremental__ : Enable the incremental global placement. Users would need to tune other parameters (e.g. init_density_penalty) with pre-placed solutions. 

### Tuning Parameters
* __bin_grid_count__ : Set bin grid's counts. Default: Defined by internal algorithm. [64,128,256,512,..., int]
* __density__ : Set target density. Default: 0.70 [0-1, float]
* __init_density_penalty__ : Set initial density penalty. Default: 8e-5 [1e-6 - 1e6, float]
* __init_wire_length__coef__ : Set initial wirelength coefficient. Default: 0.25 [unlimited, float] 
* __min_phi_coef__ : Set pcof_min(µ_k Lower Bound). Default: 0.95 [0.95-1.05, float]
* __max_phi_coef__ : Set pcof_max(µ_k Upper Bound). Default: 1.05 [1.00-1.20, float]
* __overflow__ : Set target overflow for termination condition. Default: 0.1 [0-1, float]
* __initial_place_max_iter__ : Set maximum iterations in initial place. Default: 20 [0-, int]
* __initial_place_max_fanout__ : Set net escape condition in initial place when 'fanout >= initial_place_max_fanout'. Default: 200 [1-, int]

### Other Options
* __verbose_level__ : Set verbose level for RePlAce. Default: 1 [0-10, int]

Note that all of the TCL commands are defined in the [replace.tcl](./src/replace.tcl) and [replace.i](./src/replace.i).

## Verified/supported Technologies
* ASAP 7
* GF 14
* TSMC 16 (7.5T/9T)
* ST FDSOI 28
* TSMC 45
* Fujitsu 55
* TSMC 65


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
* BSD-3-clause License [[Link]](./LICENSE)
