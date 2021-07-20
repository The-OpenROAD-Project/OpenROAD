# OpenROAD

[![Build Status](https://jenkins.openroad.tools/buildStatus/icon?job=OpenROAD-Public%2Fmaster)](https://jenkins.openroad.tools/job/OpenROAD-Public/job/master/) [![Coverity Scan Status](https://scan.coverity.com/projects/the-openroad-project-openroad/badge.svg)](https://scan.coverity.com/projects/the-openroad-project-openroad) [![Documentation Status](https://readthedocs.org/projects/openroad/badge/?version=latest)](https://openroad.readthedocs.io/en/latest/?badge=latest)

The documentation is also available [here](https://openroad.readthedocs.io/en/latest/)

## Install dependencies

For a limited number of configurations the following script can be used
to install dependencies. The script `etc/DependencyInstaller.sh` supports
Centos7 and Ubuntu 20.04. You need root access to correctly install the
dependencies with the script.

``` shell
$ ./etc/DependencyInstaller.sh -help
usage: ./etc/DependencyInstaller.sh -run[time]      # installs dependencies to run a pre-compiled binary
       ./etc/DependencyInstaller.sh -dev[elopment]  # installs dependencies to compile the openroad binary
```

## Build

The first step, independent of the build method, is to download the repository:

``` shell
git clone --recursive https://github.com/The-OpenROAD-Project/OpenROAD.git
cd OpenROAD
```

OpenROAD git submodules (cloned by the `--recursive` flag) are located in `src/`.

The default build type is RELEASE to compile optimized code.
The resulting executable is in `build/src/openroad`.

Optional CMake variables passed as `-D<var>=<value>` arguments to CMake are show below.

| Argument             | Value                     |
|----------------------|---------------------------|
| CMAKE_BUILD_TYPE     | DEBUG, RELEASE            |
| CMAKE_CXX_FLAGS      | Additional compiler flags |
| TCL_LIB              | Path to tcl library       |
| TCL_HEADER           | Path to tcl.h             |
| ZLIB_ROOT            | Path to zlib              |
| CMAKE_INSTALL_PREFIX | Path to install binary    |

### Build by hand

``` shell
$ mkdir build
$ cd build
$ cmake ..
$ make
```

The default install directory is `/usr/local`.
To install in a different directory with CMake use:

``` shell
cmake .. -DCMAKE_INSTALL_PREFIX=<prefix_path>
```

Alternatively, you can use the `DESTDIR` variable with make.

``` shell
make DESTDIR=<prefix_path> install
```

### Build using support script

``` shell
$ ./etc/Build.sh
# To build with debug option enabled and if the Tcl library is not on the default path
$ ./etc/Build.sh --cmake="-DCMAKE_BUILD_TYPE=DEBUG -DTCL_LIB=/path/to/tcl/lib"
```

The default install directory is `/usr/local`.
To install in a different directory use:

``` shell
$ ./etc/Build.sh --cmake="-DCMAKE_INSTALL_PREFIX=<prefix_path>"
```

## Regression Tests

There are a set of regression tests in `test/`.

``` shell
# run all tool unit tests
test/regression
# run all flow tests
test/regression flow
# run <tool> tests
test/regression <tool>
# run <tool> tool tests
src/<tool>/test/regression
```

The flow tests check results such as worst slack against reference values.
Use `report_flow_metrics [test]...` to see the all of the metrics.
Use `save_flow_metrics [test]...` to add margins to the metrics and save them to <test>.metrics_limits.

``` text
% report_flow_metrics gcd_nangate45
                       insts    area util slack_min slack_max  tns_max clk_skew max_slew max_cap max_fanout DPL ANT drv
gcd_nangate45            368     564  8.8     0.112    -0.015     -0.1    0.004        0       0          0   0   0   0
```

## Run

``` text
openroad
  -help              show help and exit
  -version           show version and exit
  -no_init           do not read .openroad init file
  -no_splash         do not show the license splash at startup
  -threads count     max number of threads to use
  -exit              exit after reading cmd_file
  cmd_file           source cmd_file
```

OpenROAD sources the Tcl command file `~/.openroad` unless the command
line option `-no_init` is specified.

OpenROAD then sources the command file cmd_file if it is specified on
the command line. Unless the `-exit` command line flag is specified it
enters and interactive Tcl command interpreter.

Below is a list of the available tools/modules included in the OpenROAD app.

### OpenROAD (global commands)

```
repair_timing [-setup]
              [-hold]
              [-slack_margin slack_margin]
              [-allow_setup_violations]
              [-max_utilization util]
              [-max_buffer_percent buffer_percent]
```
The `repair_timing` command repairs setup and hold violations.
It should be run after clock tree synthesis with propagated clocks.
While repairing hold violations buffers are not inserted that will cause setup
violations unless '-allow_setup_violations' is specified.
Use `-slack_margin` to add additional slack margin. To specify
different slack margins use separate `repair_timing` commands for setup and
hold. Use `-max_buffer_percent` to specify a maximum number of buffers to
insert to repair hold violations as a percent of the number of instances
in the design. The default value for `buffer_percent` is 20, for 20%.

```
report_design_area
```
The `report_design_area` command reports the area of the design's
components and the utilization.

```
report_floating_nets [-verbose]
```
The `report_floating_nets` command reports nets with only one pin connection.
Use the `-verbose` flag to see the net names.

A typical resizer command file (after a design and liberty libraries
have been read) is shown below.

```
read_sdc gcd.sdc

set_wire_rc -layer metal2

set_dont_use {CLKBUF_* AOI211_X1 OAI211_X1}

buffer_ports
repair_design -max_wire_length 100
repair_tie_fanout LOGIC0_X1/Z
repair_tie_fanout LOGIC1_X1/Z
# clock tree synthesis...
repair_timing
```

Note that OpenSTA commands can be used to report timing metrics before
or after resizing the design.

```
set_wire_rc -layer metal2
report_checks
report_tns
report_wns
report_checks

repair_design

report_checks
report_tns
report_wns
```

#### Timing Analysis

Timing analysis commands are documented in src/OpenSTA/doc/OpenSTA.pdf.

After the database has been read from LEF/DEF, Verilog or an OpenDB
database, use the `read_liberty` command to read Liberty library files
used by the design.

The example script below timing analyzes a database.

```
read_liberty liberty1.lib
read_db reg1.db
create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}
set_output_delay -clock clk 0 out
report_checks
```

#### Clock Tree Synthesis

TritonCTS 2.0 is available under the OpenROAD app as ``clock_tree_synthesis`` command.
The following tcl snippet shows how to call TritonCTS. TritonCTS 2.0 performs on-the-fly characterization.
Thus there is no need to generate characterization data. On-the-fly characterization feature could still
be optionally controlled by parameters specified to configure_cts_characterization command.
Use set_wire_rc command to set clock routing layer.

```
read_sdc "design.sdc"
set_wire_rc -clock -layer metal5

configure_cts_characterization [-max_slew <max_slew>] \
                               [-max_cap <max_cap>] \
                               [-slew_inter <slew_inter>] \
                               [-cap_inter <cap_inter>]

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


write_def "final.def"
```
Argument description:
- ``-buf_list`` are the master cells (buffers) that will be considered when making the wire segments.
- ``-root_buffer`` is the master cell of the buffer that serves as root for the clock tree. 
If this parameter is omitted, the first master cell from ``-buf_list`` is taken.
- ``-max_slew`` is the max slew value (in seconds) that the characterization will test. 
If this parameter is omitted, the code would use max slew value for specified buffer in buf_list from liberty file.
- ``-max_cap`` is the max capacitance value (in farad) that the characterization will test. 
If this parameter is omitted, the code would use max cap value for specified buffer in buf_list from liberty file.
- ``-slew_inter`` is the time value (in seconds) that the characterization will consider for results. 
If this parameter is omitted, the code gets the default value (5.0e-12). Be careful that this value can be quite low for bigger technologies (>65nm).
- ``-cap_inter`` is the capacitance value (in farad) that the characterization will consider for results. 
If this parameter is omitted, the code gets the default value (5.0e-15). Be careful that this value can be quite low for bigger technologies (>65nm).
- ``-wire_unit`` is the minimum unit distance between buffers for a specific wire. 
If this parameter is omitted, the code gets the value from ten times the height of ``-root_buffer``.
- ``-clk_nets`` is a string containing the names of the clock roots. 
If this parameter is omitted, TritonCTS looks for the clock roots automatically.
- ``-out_path`` is the output path (full) that the lut.txt and sol_list.txt files will be saved. This is used to load an existing characterization, without creating one from scratch.
- ``-post_cts_disable`` is a flag that, when specified, disables the post-processing operation for outlier sinks (buffer insertion on 10% of the way between source and sink). 
- ``-distance_between_buffers`` is the distance (in micron) between buffers that TritonCTS should use when creating the tree. When using this parameter, the clock tree algorithm is simplified, and only uses a fraction of the segments from the LUT.
- ``-branching_point_buffers_distance`` is the distance (in micron) that a branch has to have in order for a buffer to be inserted on a branch end-point. This requires the ``-distance_between_buffers`` value to be set.
- ``-clustering_exponent`` is a value that determines the power used on the difference between sink and means on the CKMeans clustering algorithm. If this parameter is omitted, the code gets the default value (4).
- ``-clustering_unbalance_ratio`` is a value that determines the maximum capacity of each cluster during CKMeans. A value of 50% means that each cluster will have exactly half of all sinks for a specific region (half for each branch). If this parameter is omitted, the code gets the default value (0.6).
- ``-sink_clustering_enable`` enables pre-clustering of sinks to create one level of sub-tree before building H-tree. Each cluster is driven by buffer which becomes end point of H-tree structure.
- ``-sink_clustering_size`` specifies the maximum number of sinks per cluster. Default value is 20.
- ``sink_clustering_max_diameter`` specifies maximum diameter (in micron) of sink cluster. Default value is 50.
- ``-clk_nets`` is a string containing the names of the clock roots. 
If this parameter is omitted, TritonCTS looks for the clock roots automatically.

Another command available from TritonCTS is ``report_cts``. It is used to extract metrics after a successful ``clock_tree_synthesis`` run. These are: Number of Clock Roots, Number of Buffers Inserted, Number of Clock Subnets, and Number of Sinks.
The following tcl snippet shows how to call ``report_cts``.

```
clock_tree_synthesis -root_buf "BUF_X4" \
                     -buf_list "BUF_X4" \
                     -wire_unit 20 

report_cts [-out_file "file.txt"]
```
``-out_file`` (optional) is the file containing the TritonCTS reports.
If this parameter is omitted, the metrics are shown on the standard output.

#### Global Routing

Global router options and commands are described below. 

```
global_route [-guide_file out_file] \
             [-verbose verbose] \
             [-congestion_iterations iterations] \
             [-grid_origin {x y}] \
             [-allow_congestion]

```
Options description:
- **guide_file**: Set the output guides file name (e.g.: -guide_file route.guide")
- **verbose**: Set verbose of report. 0 for less verbose, 1 for medium verbose, 2 for full verbose (e.g.: -verbose *1*)
- **congestion_iterations**: Set the number of iterations to remove the overflow of the routing (e.g.: -congestion_iterations *50*)
- **grid_origin**: Set the origin of the routing grid (e.g.: -grid_origin {1 1})
- **allow_congestion**: Allow global routing results with congestion

```
set_routing_layers [-signal min-max] \
                   [-clock min-max]
```
The `set_routing_layers` command sets the minimum and maximum routing layers for signal nets, with the `-signal` option,
and the the minimum and maximum routing layers for clock nets, with the `-clock` option
Example: `set_routing_layers -signal Metal2-Metal10 -clock Metal6-Metal9`

```
set_macro_extension extension
```
The `set_macro_extension` command sets the number of GCells added to the blocakges boundaries from macros
Example: `set_macro_extension 2`

```
set_global_routing_layer_adjustment layer adjustment
```
The `set_global_routing_layer_adjustment` command sets routing resources adjustments in the routing layers of the design.
You can set adjustment for a specific layer, e.g.: `set_global_routing_layer_adjustment Metal4 0.5` reduces the routing resources
of routing layer Metal4 in 50%.
You can set adjustment for all layers at once using `*`, e.g.: `set_global_routing_layer_adjustment * 0.3` reduces
the routing resources of all routing layers in 30%.
You can set adjustment for a layer range, e.g.: `set_global_routing_layer_adjustment Metal4-Metal8 0.3` reduces
the routing resources of routing layers  Metal4, Metal5, Metal6, Metal7 and Metal8 in 30%.

```
set_global_routing_layer_pitch layer pitch
```
The `set_global_routing_layer_pitch` command sets the pitch for routing tracks in a specific layer.
You can call it multiple times for different layers.
Example: `set_global_routing_layer_pitch Metal6 1.34`.

```
set_routing_alpha [-net net_name] alpha
```
By default the global router uses steiner trees to construct route guides. A steiner tree minimizes the total wire length.
Prim/Dijkstra is an alternative net topology algorithm that supports a trade-off between total wire length and maximum path depth from
the net driver to its loads.
The `set_routing_alpha` command enables the Prim/Dijkstra algorithm and sets the alpha parameter used to trade-off wire length and path depth.
Alpha is between 0.0 and 1.0. When alpha is 0.0 the net topology minimizes total wire length (i.e. capacitance).
When alpha is 1.0 it minimizes longest path between the driver and loads (i.e., maximum resistance).
Typical values are 0.4-0.8. For more information about PDRev, check the paper in `src/FastRoute/src/pdrev/papers/PDRev.pdf`
You can call it multiple times for different nets.
Example: `set_routing_alpha -net clk 0.3` sets the alpha value of 0.3 for net *clk*.

```
set_global_routing_region_adjustment {lower_left_x lower_left_y upper_right_x upper_right_y}
                                     -layer layer -adjustment adjustment
```
The `set_global_routing_region_adjustment` command sets routing resources adjustments in a specific region of the design.
The region is defined as a rectangle in a routing layer.
Example: `set_global_routing_region_adjustment {1.5 2 20 30.5}
                                               -layer Metal4 -adjustment 0.7`

```
set_global_routing_random [-seed seed] \
                          [-capacities_perturbation_percentage percent] \
                          [-perturbation_amount value]
```
The `set_global_routing_random` command enables random global routing results. The random global routing shuffles the order
of the nets and randomly subtracts or add the capacities of a random set of edges.
The `-seed` option sets the random seed and is required to enable random mode. The `-capacities_perturbation_percentage` option
sets the percentage of edges to perturb the capacities. By default, the edge capacities are perturbed by sum or subtract 1 from the original capacity.
The `-perturbation_amount` option sets the perturbation value of the edge capacities. This option will only have effect when `-capacities_perturbation_percentage`
is used.
The random seed must be different from 0 to enable random global routing.
Example: `set_global_routing_random -seed 42 -capacities_perturbation_percentage 50 -perturbation_amount 2`

```
repair_antennas diodeCellName/diodePinName [-iterations iterations]
```
The repair_antenna command evaluates the global routing results looking for antenna violations, and repairs the violations
by inserting diodes. The input for this command is the diode cell and pin names and the number of iterations. By default,
the command runs only one iteration to repair antennas.
It uses the  `antennachecker` tool to identify the antenna violations and return the exact number of diodes necessary to
fix the antenna violation.
Example: `repair_antenna sky130_fd_sc_hs__diode_2/DIODE`

```
write_guides file_name
```
The `write_guides` generates the guide file from the routing results.
Example: `write_guides route.guide`.

To estimate RC parasitics based on global route results, use the `-global_routing`
option of the `estimate_parasitics` command.

```
estimate_parasitics -global_routing
```

```
draw_route_guides net_names
```
The `draw_route_guides` command plots the route guides for a set of nets.
To erase the route guides from the GUI, pass an empty list to this command: `draw_route_guides {}`.


#### PDN analysis

PDNSim PDN checker searches for floating PDN stripes on the power and ground nets. 

PDNSim reports worst IR drop and worst current density in a power wire drop given a placed and PDN synthesized design.

PDNSim spice netlist writer for power wires.

Commands for the above three functionalities are below: 

```
set_pdnsim_net_voltage -net <net_name> -voltage <voltage_value>
check_power_grid -net <net_name>
analyze_power_grid -vsrc <voltage_source_location_file> \
                   -net <net_name> \ 
                   [-outfile <filename>] \
                   [-enable_em] \
                   [-em_outfile <filename>]
                   [-dx]
                   [-dy]
                   [-em_outfile <filename>]
write_pg_spice -vsrc <voltage_source_location_file> -outfile <netlist.sp> -net <net_name>
```

Options description:
- **vsrc**: (optional) file to set the location of the power C4 bumps/IO pins.
        [Vsrc_aes.loc file](https://github.com/The-OpenROAD-Project/PDNSim/blob/master/test/aes/Vsrc.loc) 
        for an example with a description specified [here](https://github.com/The-OpenROAD-Project/PDNSim/blob/master/doc/Vsrc_description.md).
- **dx,dy**: (optional) these arguments set the bump pitch to decide the voltage
  source location in the absence of a vsrc file. Default bump pitch of 140um
  used in absence of these arguments and vsrc 
- **net**: (mandatory) is the name of the net to analyze, power or ground net name
- **enable_em**: (optional) is the flag to report current per power grid segment
- **outfile**: (optional) filename specified per-instance voltage written into file
- **em_outfile**: (optional) filename to write out the per segment current values into a file, 
  can be specified only if enable_em is flag exists
- **voltage**: Sets the voltage on a specific net. If this command is not run,
  the voltage value is obtained from operating conditions in the liberty.

###### Note: See the file [Vsrc_aes.loc file](https://github.com/The-OpenROAD-Project/PDNSim/blob/master/test/aes/Vsrc.loc) for an example with a description specified [here](https://github.com/The-OpenROAD-Project/PDNSim/blob/master/doc/Vsrc_description.md).

#### TCL functions

Get the die and core areas as a list in microns: "llx lly urx ury"

## License

[BSD 3-Clause](./LICENSE)
