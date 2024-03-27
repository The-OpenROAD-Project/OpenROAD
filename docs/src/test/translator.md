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

Routability-driven arguments
- They begin with `-routability`.
- `-routability_check_overflow`, `-routability_max_density`, `-routability_max_bloat_iter`, `-routability_max_inflation_iter`, `-routability_target_rc_metric`, `-routability_inflation_ratio_coef`, `-routability_max_inflation_ratio`, `-routability_rc_coefficients`

Timing-driven arguments
- They begin with `-timing_driven`.
- `-timing_driven_net_reweight_overflow`, `-timing_driven_net_weight_max`, `-timing_driven_nets_percentage`

```tcl
global_placement
    [-timing_driven]
    [-routability_driven]
    [-disable_timing_driven]
    [-disable_routability_driven]
    [-skip_initial_place]
    [-incremental]
    [-bin_grid_count grid_count]
    [-density target_density]
    [-init_density_penalty init_density_penalty]
    [-init_wirelength_coef init_wirelength_coef]
    [-min_phi_coef min_phi_conef]
    [-max_phi_coef max_phi_coef]
    [-reference_hpwl reference_hpwl]
    [-overflow overflow]
    [-initial_place_max_iter initial_place_max_iter]
    [-initial_place_max_fanout initial_place_max_fanout]
    [-pad_left pad_left]
    [-pad_right pad_right]
    [-force_cpu]
    [-skip_io]
    [-skip_nesterov_place]
    [-routability_check_overflow routability_check_overflow]
    [-routability_max_density routability_max_density]
    [-routability_max_bloat_iter routability_max_bloat_iter]
    [-routability_max_inflation_iter routability_max_inflation_iter]
    [-routability_target_rc_metric routability_target_rc_metric]
    [-routability_inflation_ratio_coef routability_inflation_ratio_coef]
    [-routability_max_inflation_ratio routability_max_inflation_ratio]
    [-routability_rc_coefficients routability_rc_coefficients]
    [-timing_driven_net_reweight_overflow]
    [-timing_driven_net_weight_max]
    [-timing_driven_nets_percentage]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-timing_driven` | Enable timing-driven mode. See [link](#timing-driven-arguments) for timing-specific arguments. |
| `-routability_driven` | Enable routability-driven mode. See [link](#routability-driven-arguments) for routability-specific arguments. |
| `-skip_initial_place` | Skip the initial placement (Biconjugate gradient stabilized, or BiCGSTAB solving) before Nesterov placement. Initial placement improves HPWL by ~5% on large designs. Equivalent to `-initial_place_max_iter 0`. | 
| `-incremental` | Enable the incremental global placement. Users would need to tune other parameters (e.g., `init_density_penalty`) with pre-placed solutions. | 
| `-bin_grid_count` | Set bin grid's counts. The internal heuristic defines the default value. Allowed values are integers `[64,128,256,512,...]`. |
| `-density` | Set target density. The default value is `0.7` (i.e., 70%). Allowed values are floats `[0, 1]`. |
| `-init_density_penalty` | Set initial density penalty. The default value is `8e-5`. Allowed values are floats `[1e-6, 1e6]`. |
| `-init_wirelength_coef` | Set initial wirelength coefficient. The default value is `0.25`. Allowed values are floats. |
| `-min_phi_coef` | Set `pcof_min` ($\mu_k$ Lower Bound). The default value is `0.95`. Allowed values are floats `[0.95, 1.05]`. |
| `-max_phi_coef` | Set `pcof_max` ($\mu_k$ Upper Bound). Default value is 1.05. Allowed values are `[1.00-1.20, float]`. |
| `-overflow` | Set target overflow for termination condition. The default value is `0.1`. Allowed values are floats `[0, 1]`. |
| `-initial_place_max_iter` | Set maximum iterations in the initial place. The default value is 20. Allowed values are integers `[0, MAX_INT]`. |
| `-initial_place_max_fanout` | Set net escape condition in initial place when $fanout \geq initial\_place\_max\_fanout$. The default value is 200. Allowed values are integers `[1, MAX_INT]`. |
| `-pad_left` | Set left padding in terms of number of sites. The default value is 0, and the allowed values are integers `[1, MAX_INT]` |
| `-pad_right` | Set right padding in terms of number of sites. The default value is 0, and the allowed values are integers `[1, MAX_INT]` |
| `-force_cpu` | Force to use the CPU solver even if the GPU is available. |
| `-skip_io` | Flag to ignore the IO ports when computing wirelength during placement. The default value is False, allowed values are boolean. |

#### Routability-Driven Arguments

| Switch Name | Description |
| ----- | ----- |
| `-routability_check_overflow` | Set overflow threshold for routability mode. The default value is `0.2`, and the allowed values are floats `[0, 1]`. |
| `-routability_max_density` | Set density threshold for routability mode. The default value is `0.99`, and the allowed values are floats `[0, 1]`. |
| `-routability_max_bloat_iter` | Set bloat iteration threshold for routability mode. The default value is `1`, and the allowed values are integers `[1, MAX_INT]`.|
| `-routability_max_inflation_iter` | Set inflation iteration threshold for routability mode. The default value is `4`, and the allowed values are integers `[1, MAX_INT]`. |
| `-routability_target_rc_metric` | Set target RC metric for routability mode. The default value is `1.25`, and the allowed values are floats. |
| `-routability_inflation_ratio_coef` | Set inflation ratio coefficient for routability mode. The default value is `2.5`, and the allowed values are floats. |
| `-routability_max_inflation_ratio` | Set inflation ratio threshold for routability mode. The default value is `2.5`, and the allowed values are floats. |
| `-routability_rc_coefficients` | Set routability RC coefficients. It comes in the form of a Tcl List `{k1, k2, k3, k4}`. The default value for each coefficient is `{1.0, 1.0, 0.0, 0.0}` respectively, and the allowed values are floats. |

#### Timing-Driven Arguments

| Switch Name | Description |
| ----- | ----- |
| `-timing_driven_net_reweight_overflow` | Set overflow threshold for timing-driven net reweighting. Allowed value is a Tcl list of integers where each number is `[0, 100]`. |
| `-timing_driven_net_weight_max` | Set the multiplier for the most timing-critical nets. The default value is `1.9`, and the allowed values are floats. |
| `-timing_driven_nets_percentage` | Set the reweighted percentage of nets in timing-driven mode. The default value is 10. Allowed values are floats `[0, 100]`. |

### Cluster Flops

This command does flop clustering based on parameters.

```tcl
cluster_flops
    [-tray_weight tray_weight]\
    [-timing_weight timing_weight]\
    [-max_split_size max_split_size]\
    [-num_paths num_paths]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-tray_weight` | Tray weight, default value is 20.0, type `float`. |
| `-timing_weight` | Timing weight, default value is 1.0, type `float`. |
| `-max_split_size` | Maximum split size, default value is -1, type `int`.|
| `-num_paths` | KIV, default value is 0, type `int`. |


## Useful Developer Commands

If you are a developer, you might find these useful. More details can be found in the [source file](./src/replace.cpp) or the [swig file](./src/replace.i).

```
# debugging global placement 
global_placement_debug -pause -update -inst -draw_bins -initial

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
that is located [here](include/gpl/Replace.h).

When initializing a design, a sequence of Python commands might look like
the following:

```python
from openroad import Design, Tech
tech = Tech()
tech.readLef(...)
design = Design(tech)
design.readDef(...)
gpl = design.getReplace()
```    

Here is an example of some options / configurations to the global placer.
(See [Replace.h](include/gpl/Replace.h) for a complete list)

```python
gpl.setInitialPlaceMaxIter(iter)
gpl.setSkipIoMode(skip_io)
gpl.setTimingDrivenMode(timing_driven)
gpl.setTimingNetWeightMax(weight)
```

There are some useful Python functions located in the file
[grt_aux.py](test/grt_aux.py) but these are not considered a part of the *final*
API and they may change.

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+replace+in%3Atitle)
about this tool.

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