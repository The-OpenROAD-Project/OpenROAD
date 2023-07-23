# TritonCTS 2.0

TritonCTS 2.0 is available under the OpenROAD app as `clock_tree_synthesis`
command.  TritonCTS 2.0 performs on-the-fly characterization. Thus there is
no need to generate characterization data. On-the-fly characterization feature
could still be optionally controlled by parameters specified to 
`configure_cts_characterization` command. Use `set_wire_rc` command to 
set clock routing layer.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Configure CTS Characterization

```tcl
configure_cts_characterization 
    [-max_slew <max_slew>]
    [-max_cap <max_cap>]
    [-slew_steps <slew_steps>]
    [-cap_steps <cap_steps>]
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-max_slew` | max slew value (in seconds) that the characterization will test. If this parameter is omitted, the code would use max slew value for specified buffer in `buf_list` from liberty file. |
| `-max_cap` | max capacitance value (in farad) that the characterization will test. If this parameter is omitted, the code would use max cap value for specified buffer in `buf_list` from liberty file. |
| `-slew_steps` | number of steps that `max_slew` will be divided into for characterization. (default 12) |
| `-cap_steps` | number of steps that `max_cap` will be divided into for characterization. (default 34) |

### Clock Tree Synthesis

```tcl
clock_tree_synthesis 
    -buf_list <list_of_buffers>
    [-root_buf <root_buf>]
    [-wire_unit <wire_unit>]
    [-clk_nets <list_of_clk_nets>]
    [-distance_between_buffers]
    [-branching_point_buffers_distance]
    [-clustering_exponent]
    [-clustering_unbalance_ratio]
    [-sink_clustering_enable]
    [-sink_clustering_size <cluster_size>]
    [-sink_clustering_max_diameter <max_diameter>]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-buf_list` | master cells (buffers) that will be considered when making the wire segments. |
| `-root_buffer` | the master cell of the buffer that serves as root for the clock tree. If this parameter is omitted, the first master cell from `-buf_list` is taken. |
| `-wire_unit` | minimum unit distance between buffers for a specific wire. If this parameter is omitted, the code gets the value from ten times the height of `-root_buffer`. |
| `-clk_nets` | string containing the names of the clock roots. If this parameter is omitted, TritonCTS looks for the clock roots automatically. |
| `-distance_between_buffers` | distance (in micron) between buffers that TritonCTS should use when creating the tree. When using this parameter, the clock tree algorithm is simplified, and only uses a fraction of the segments from the LUT. |
| `-branching_point_buffers_distance` | distance (in micron) that a branch has to have in order for a buffer to be inserted on a branch end-point. This requires the `-distance_between_buffers` value to be set. |
| `-clustering_exponent` | value that determines the power used on the difference between sink and means on the CKMeans clustering algorithm. (default 4) |
| `-clustering_unbalance_ratio` | value that determines the maximum capacity of each cluster during CKMeans. A value of 50% means that each cluster will have exactly half of all sinks for a specific region (half for each branch). (default 0.6) |
| `-sink_clustering_enable` | enables pre-clustering of sinks to create one level of sub-tree before building H-tree. Each cluster is driven by buffer which becomes end point of H-tree structure. |
| `-sink_clustering_size` | specifies the maximum number of sinks per cluster. (default 20) |
| `sink_clustering_max_diameter` | specifies maximum diameter (in micron) of sink cluster. (default 50) |
| `-clk_nets` | string containing the names of the clock roots. If this parameter is omitted, TritonCTS looks for the clock roots automatically. |

### Report CTS

Another command available from TritonCTS is `report_cts`. It is used to
extract metrics after a successful `clock_tree_synthesis` run. These are:
 
- Number of Clock Roots
- Number of Buffers Inserted
- Number of Clock Subnets
- Number of Sinks.  

```tcl
report_cts [-out_file <file>]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-out_file` | the file containing the TritonCTS reports. If this parameter is omitted, the metrics are shown onto `stdout` and not saved. |

## Example scripts

```tcl
clock_tree_synthesis -root_buf "BUF_X4" \
                     -buf_list "BUF_X4" \
                     -wire_unit 20
report_cts "file.txt"
```

## Regression tests

There are a set of regression tests in `/test`. For more information, refer to this [section](../../README.md#regression-tests).

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
