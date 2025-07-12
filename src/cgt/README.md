# Clock gating

This module provides automatic insertion of clock gates for reducing power usage.
It uses ABC for proving correctness of gating conditions.

## Commands

```{note}
All parameters for clock gating are optional, as indicated by square brackets: `[-param param]`.
```

### Clock gating

```tcl
clock_gating
    [-instances instances]
    [-gate_cond_nets gate_cond_nets]
    [-min_instances min_instances]
    [-max_cover max_cover]
    [-group_instances group_instances]
    [-dump_dir dump_dir]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-instances` | List of instances to be gated (by default all clocked instances). |
| `-gate_cond_nets` | List of candidate nets for gating conditions.
| `-min_instances` | Minimum number of instances that should be gated by a single clock gate. |
| `-max_cover` | Maximum number of initial gate condition candidate nets per instance (or instance group). |
| `-group_instances` | Whether to group instances together when checking a gate condition. Possible values: `cell_prefix` (group by instance name prefix) or `net_prefix` (group by net name prefix). |
| `-dump_dir` | Name of the directory for debug dumps. |

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

Clock gating is not supported for designs that contain cells not supported by ABC, such as adders.

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+clock-gating)
about this tool.

## References

1. Aaron P. Hurst. 2008. Automatic synthesis of clock gating logic with controlled netlist perturbation. In Proceedings of the 45th annual Design Automation Conference (DAC '08). Association for Computing Machinery, New York, NY, USA, 654–657. https://doi.org/10.1145/1391469.1391637

