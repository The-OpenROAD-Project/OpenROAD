# Clock gating

This module provides automatic insertion of clock gates for reducing power usage.
It uses ABC for proving correctness of gating conditions.

The process is roughly:
1. For each register:
  a. Gather nets connected to the register via BFS (limited to 100 nets for performance, user configurable).
     If it encounters a register, it doesn't go through it; it stops there and goes in different directions.
  b. Check if a previously accepted gating condition can be reused; if so, add this register to that condition's list of registers.
  c. Export the network gathered in (a) to ABC.
  d. Check if all the nets in the exported network can form a correct gating condition using ABC.
     - First, simulation with random stimuli quickly looks for counterexamples.
     - Then, if no counterexample was found, a SAT solver is employed to prove that the gating condition is correct.
     A clock enable condition is checked by ORing the nets, and a clock disable condition by ANDing them.
  e. If the set of all nets doesn't form a correct gating condition, move on to the next register.
     Otherwise:
     - Check if after removing half of the nets the gating condition still works.
     - If so, drop the other half of the nets. Otherwise, recurse into the other half of the nets to minimize that subset.
     - Then, recurse into the first half of the nets to minimize that part.
     This produces a minimal set of nets that form a gating condition (not necessarily optimal).
  f. Add the minimal set of nets with the corresponding gated register to a list of accepted gating conditions.
     First check if it doesn't contain it already; if it does, add the gated register to the pre-existing condition's list of registers.
2. For each accepted gating condition with at least 10 corresponding registers (user-configurable),
   insert a new clock gate that gates the corresponding registers under this condition.

Usage:

```tcl
read_liberty path/to/pdk/cell/library.lib
read_db path/to/your/design.odb
read_sdc path/to/your/constraints.sdc

clock_gating
```

## Commands

```{note}
All parameters for clock gating are optional, as indicated by square brackets: `[-param param]`.
```

### Clock gating

```tcl
clock_gating
    [-min_instances min_instances]
    [-max_cover max_cover]
    [-dump_dir dump_dir]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-min_instances` | Minimum number of instances that should be gated by a single clock gate. |
| `-max_cover` | Maximum number of initial gate condition candidate nets per instance. |
| `-dump_dir` | Directory for debug dumps. |

## Example scripts

Example script on running `cgt` for a sample design of `aes` can be found here:

```
./test/aes_nangate45.tcl
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following script:

```shell
./test/regression
```

## Limitations

Clock gating is currently not available for designs that contain HA and FA cells which are not supported by ABC.

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+clock-gating)
about this tool.

## References

1. Aaron P. Hurst. 2008. Automatic synthesis of clock gating logic with controlled netlist perturbation. In Proceedings of the 45th annual Design Automation Conference (DAC '08). Association for Computing Machinery, New York, NY, USA, 654–657. https://doi.org/10.1145/1391469.1391637

