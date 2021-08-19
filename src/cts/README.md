# TritonCTS 2.0

TritonCTS 2.0 is available under the OpenROAD app as `clock_tree_synthesis`
command.  The following tcl snippet shows how to call TritonCTS. TritonCTS
2.0 performs on-the-fly characterization.  Thus there is no need to
generate characterization data. On-the-fly characterization feature
could still be optionally controlled by parameters specified to
`configure_cts_characterization` command.  Use `set_wire_rc` command to
set clock routing layer.

## Commands

### Configure CTS Characterization

```
configure_cts_characterization [-max_slew <max_slew>] \
                               [-max_cap <max_cap>] \
                               [-slew_inter <slew_inter>] \
                               [-cap_inter <cap_inter>]
```

Argument description:

-   `-max_slew` is the max slew value (in seconds) that the characterization
    will test. If this parameter is omitted, the code would use max slew value
    for specified buffer in `buf_list` from liberty file.
-   `-max_cap` is the max capacitance value (in farad) that the
    characterization will test. If this parameter is omitted, the code would
    use max cap value for specified buffer in `buf_list` from liberty file.
-   `-slew_inter` is the time value (in seconds) that the characterization
    will consider for results. If this parameter is omitted, the code gets the
    default value (`5.0e-12`). Be careful that this value can be quite low for
    bigger technologies (`>65nm`).
-   `-cap_inter` is the capacitance value (in farad) that the characterization
    will consider for results. If this parameter is omitted, the code gets the
    default value (`5.0e-15`). Be careful that this value can be quite low for
    bigger technologies (`>65nm`).


### Clock Tree Synthesis

```
clock_tree_synthesis -buf_list <list_of_buffers> \
                     [-root_buf <root_buf>] \
                     [-wire_unit <wire_unit>] \
                     [-clk_nets <list_of_clk_nets>] \
                     [-out_path <lut_path>] \
                     [-post_cts_disable] \
                     [-distance_between_buffers] \
                     [-branching_point_buffers_distance] \
                     [-clustering_exponent] \
                     [-clustering_unbalance_ratio] \
                     [-sink_clustering_enable] \
                     [-sink_clustering_size <cluster_size>] \
                     [-sink_clustering_max_diameter <max_diameter>]
```

Argument description:

-   `-buf_list` are the master cells (buffers) that will be considered when
    making the wire segments.
-   `-root_buffer` is the master cell of the buffer that serves as root for
    the clock tree. If this parameter is omitted, the first master cell from
    `-buf_list` is taken.
-   `-wire_unit` is the minimum unit distance between buffers for a specific
    wire. If this parameter is omitted, the code gets the value from ten times
    the height of `-root_buffer`.
-   `-clk_nets` is a string containing the names of the clock roots. If
    this parameter is omitted, TritonCTS looks for the clock roots automatically.
-   `-out_path` is the output path (full) that the `lut.txt` and `sol_list.txt`
    files will be saved. This is used to load an existing characterization,
    without creating one from scratch.
-   `-post_cts_disable` is a flag that, when specified, disables the
    post-processing operation for outlier sinks (buffer insertion on 10% of
    the way between source and sink).
-   `-distance_between_buffers` is the distance (in micron) between buffers
    that TritonCTS should use when creating the tree. When using this parameter,
    the clock tree algorithm is simplified, and only uses a fraction of the
    segments from the LUT.
-   `-branching_point_buffers_distance` is the distance (in micron) that
    a branch has to have in order for a buffer to be inserted on a branch
    end-point. This requires the `-distance_between_buffers` value to be set.
-   `-clustering_exponent` is a value that determines the power used on the
    difference between sink and means on the CKMeans clustering algorithm. If
    this parameter is omitted, the code gets the default value (4).
-   `-clustering_unbalance_ratio` is a value that determines the maximum
    capacity of each cluster during CKMeans. A value of 50% means that each
    cluster will have exactly half of all sinks for a specific region (half for
    each branch). If this parameter is omitted, the code gets the default value
    (0.6).
-   `-sink_clustering_enable` enables pre-clustering of sinks to create one
    level of sub-tree before building H-tree. Each cluster is driven by buffer
    which becomes end point of H-tree structure.
-   `-sink_clustering_size` specifies the maximum number of sinks per
    cluster. Default value is 20.
-   `sink_clustering_max_diameter` specifies maximum diameter (in micron)
    of sink cluster. Default value is 50.
-   `-clk_nets` is a string containing the names of the clock roots. If
    this parameter is omitted, TritonCTS looks for the clock roots automatically.


### Report CTS

Another command available from TritonCTS is `report_cts`. It is used to
extract metrics after a successful `clock_tree_synthesis` run. These
are: Number of Clock Roots, Number of Buffers Inserted, Number of Clock
Subnets, and Number of Sinks.  The following tcl snippet shows how to call
`report_cts`.

```
report_cts [-out_file <file>]
```

Argument description:

-   `-out_file` (optional) is the file containing the TritonCTS reports.
    If this parameter is omitted, the metrics are shown on the standard
    output.

## Example scripts

```
clock_tree_synthesis -root_buf "BUF_X4" \
                     -buf_list "BUF_X4" \
                     -wire_unit 20

report_cts "file.txt"
```

## Regression tests

## Limitations

## FAQs

Check out
[GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+fastroute+in%3Atitle)
about this tool.

### External references

-   [LEMON](https://lemon.cs.elte.hu/trac/lemon) - **L**ibrary for
    **E**fficient **M**odeling and **O**ptimization in **N**etworks
-   Capacitate k-means package from Dr. Jiajia Li (UCSD).  Published
    [here](https://vlsicad.ucsd.edu/Publications/Conferences/344/c344.pdf).

## Authors

TritonCTS 2.0 is written by Mateus Fogaça, PhD student in the Graduate
Program on Microelectronics from the Federal University of Rio Grande do Sul
(UFRGS), Brazil. Mr. Fogaça advisor is Prof. Ricardo Reis

Many guidance provided by (alphabetic order):
-  Andrew B. Kahng
-  Jiajia Li
-  Kwangsoo Han
-  Tom Spyrou

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
