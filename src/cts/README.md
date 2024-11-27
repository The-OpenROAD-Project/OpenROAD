# Clock Tree Synthesis

The clock tree synthesis module in OpenROAD (`cts`) is based on TritonCTS
2.0. It is available from the `clock_tree_synthesis` command. TritonCTS 2.0
performs on-the-fly characterization. Thus, there is no need to generate
characterization data. The on-the-fly characterization feature can be optionally
controlled by parameters specified by the `configure_cts_characterization`
command. Use `set_wire_rc` command to set the clock routing layer.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Configure CTS Characterization

Configure key CTS characterization parameters, for example maximum slew and capacitance,
as well as the number of steps they will be divided for characterization.

```tcl
configure_cts_characterization 
    [-max_slew max_slew]
    [-max_cap max_cap]
    [-slew_steps slew_steps]
    [-cap_steps cap_steps]
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-max_slew` | Max slew value (in the current time unit) that the characterization will test. If this parameter is omitted, the code would use max slew value for specified buffer in `buf_list` from liberty file. |
| `-max_cap` | Max capacitance value (in the current capacitance unit) that the characterization will test. If this parameter is omitted, the code would use max cap value for specified buffer in `buf_list` from liberty file. |
| `-slew_steps` | Number of steps that `max_slew` will be divided into for characterization. The default value is `12`, and the allowed values are integers `[0, MAX_INT]`. |
| `-cap_steps` | Number of steps that `max_cap` will be divided into for characterization. The default value is `34`, and the allowed values are integers `[0, MAX_INT]`. |

### Clock Tree Synthesis

Perform clock tree synthesis.

```tcl
clock_tree_synthesis 
    [-wire_unit wire_unit]
    [-buf_list <list_of_buffers>]
    [-root_buf root_buf]
    [-clk_nets <list_of_clk_nets>]
    [-tree_buf <buf>]
    [-distance_between_buffers]
    [-branching_point_buffers_distance]
    [-clustering_exponent]
    [-clustering_unbalance_ratio]
    [-sink_clustering_size cluster_size]
    [-sink_clustering_max_diameter max_diameter]
    [-sink_clustering_enable]
    [-balance_levels]
    [-sink_clustering_levels levels]
    [-num_static_layers]
    [-sink_clustering_buffer]
    [-obstruction_aware]
    [-apply_ndr]
    [-insertion_delay]
    [-dont_use_dummy_load]
    [-sink_buffer_max_cap_derate derate_value]
    [-delay_buffer_derate derate_value]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-buf_list` | Tcl list of master cells (buffers) that will be considered when making the wire segments (e.g. `{BUFXX, BUFYY}`). |
| `-root_buffer` | The master cell of the buffer that serves as root for the clock tree. If this parameter is omitted, the first master cell from `-buf_list` is taken. |
| `-wire_unit` | Minimum unit distance between buffers for a specific wire. If this parameter is omitted, the code gets the value from ten times the height of `-root_buffer`. |
| `-distance_between_buffers` | Distance (in microns) between buffers that `cts` should use when creating the tree. When using this parameter, the clock tree algorithm is simplified and only uses a fraction of the segments from the LUT. |
| `-branching_point_buffers_distance` | Distance (in microns) that a branch has to have in order for a buffer to be inserted on a branch end-point. This requires the `-distance_between_buffers` value to be set. |
| `-clustering_exponent` | Value that determines the power used on the difference between sink and means on the CKMeans clustering algorithm. The default value is `4`, and the allowed values are integers `[0, MAX_INT]`. |
| `-clustering_unbalance_ratio` | Value determines each cluster's maximum capacity during CKMeans. A value of `0.5` (i.e., 50%) means that each cluster will have exactly half of all sinks for a specific region (half for each branch). The default value is `0.6`, and the allowed values are floats `[0, 1.0]`. |
| `-sink_clustering_enable` | Enables pre-clustering of sinks to create one level of sub-tree before building H-tree. Each cluster is driven by buffer which becomes end point of H-tree structure. |
| `-sink_clustering_size` | Specifies the maximum number of sinks per cluster. The default value is `20`, and the allowed values are integers `[0, MAX_INT]`. |
| `-sink_clustering_max_diameter` | Specifies maximum diameter (in microns) of sink cluster. The default value is `50`, and the allowed values are integers `[0, MAX_INT]`. |
| `-balance_levels` | Attempt to keep a similar number of levels in the clock tree across non-register cells (e.g., clock-gate or inverter). The default value is `False`, and the allowed values are bool. |
| `-clk_nets` | String containing the names of the clock roots. If this parameter is omitted, `cts` looks for the clock roots automatically. |
| `-num_static_layers` | Set the number of static layers. The default value is `0`, and the allowed values are integers `[0, MAX_INT]`. |
| `-sink_clustering_buffer` | Set the sink clustering buffer(s) to be used. |
| `-obstruction_aware` | Enables obstruction-aware buffering such that clock buffers are not placed on top of blockages or hard macros. This option may reduce legalizer displacement, leading to better latency, skew or timing QoR.  The default value is `False`, and the allowed values are bool. |
| `-apply_ndr` | Applies 2X spacing non-default rule to all clock nets except leaf-level nets. The default value is `False`. |
| `-dont_use_dummy_load` | Don't apply dummy buffer or inverter cells at clock tree leaves to balance loads. The default values is `False`. |
| `-sink_buffer_max_cap_derate` | Use this option to control automatic buffer selection. To favor strong(weak) drive strength buffers use a small(large) value.  The default value is `0.01`, meaning that buffers are selected by derating max cap limit by 0.01. The value of 1.0 means no derating of max cap limit.  |
| `-delay_buffer_derate` | This option balances latencies between macro cells and registers by inserting delay buffers.  The default value is `1.0`, meaning all needed delay buffers are inserted.  A value of 0.5 means only half of necessary delay buffers are inserted.  A value of 0.0 means no insertion of delay buffers. |
| `-library` | This option specifies the name of library from which clock buffers will be selected, such as the LVT or uLVT library.  It is assumed that the library has already been loaded using the read_liberty command.  If this option is not specified, clock buffers will be chosen from the currently loaded libraries, which may not include LVT or uLVT cells. |

### Report CTS

This command is used to extract the following metrics after a successful `clock_tree_synthesis` run. 
- Number of Clock Roots
- Number of Buffers Inserted
- Number of Clock Subnets
- Number of Sinks.  

```tcl
report_cts 
    [-out_file file]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-out_file` | The file to save `cts` reports. If this parameter is omitted, the report is streamed to `stdout` and not saved. |

## Useful Developer Commands

If you are a developer, you might find these useful. More details can be found in the [source file](./src/TritonCTS.cpp) or the [swig file](./src/TritonCTS.i).

| Command Name | Description |
| ----- | ----- | 
| `clock_tree_synthesis_debug` | Option to plot the CTS to GUI. |

## Example scripts

```
clock_tree_synthesis -root_buf "BUF_X4" \
                     -buf_list "BUF_X4" \
                     -wire_unit 20
report_cts "file.txt"
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following script:

```shell
./test/regression
```

## Limitations

## FAQs

Check out
[GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+cts) about this tool.

## References

1.   [LEMON](https://lemon.cs.elte.hu/trac/lemon) - **L**ibrary for
    **E**fficient **M**odeling and **O**ptimization in **N**etworks
1.  Kahng, A. B., Li, J., & Wang, L. (2016, November). Improved flop tray-based design implementation for power reduction. In 2016 IEEE/ACM International Conference on Computer-Aided Design (ICCAD) (pp. 1-8). IEEE. [(.pdf)](https://vlsicad.ucsd.edu/Publications/Conferences/344/c344.pdf)

## Authors

TritonCTS 2.0 is written by Mateus Fogaça, PhD student in the Graduate
Program on Microelectronics from the Federal University of Rio Grande do Sul
(UFRGS), Brazil. Mr. Fogaça's advisor is Prof. Ricardo Reis.

Many guidance provided by (alphabetic order):
-  Andrew B. Kahng
-  Jiajia Li
-  Kwangsoo Han
-  Tom Spyrou

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.

