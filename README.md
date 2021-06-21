# OpenROAD

[![Build Status](https://jenkins.openroad.tools/buildStatus/icon?job=OpenROAD-Public%2Fmaster)](https://jenkins.openroad.tools/job/OpenROAD-Public/job/master/) [![Coverity Scan Status](https://scan.coverity.com/projects/the-openroad-project-openroad/badge.svg)](https://scan.coverity.com/projects/the-openroad-project-openroad) [![Documentation Status](https://readthedocs.org/projects/openroad/badge/?version=latest)](https://openroad.readthedocs.io/en/latest/?badge=latest)

OpenROAD is an integrated chip physical design tool that takes a
design from synthesized Verilog to routed layout.  


An outline of steps used to build a chip using OpenROAD are shown below.

* Initialize floorplan - define the chip size and cell rows
* Place pins (for designs without pads )
* Place macro cells (RAMs, embedded macros)
* Insert substrate tap cells
* Insert power distribution network
* Macro Placement of macro cells
* Global placement of standard cells
* Repair max slew, max capacitance, and max fanout violations and long wires
* Clock tree synthesis
* Optimize setup/hold timing
* Insert fill cells
* Global routing (route guides for detailed routing)
* Detailed routing

OpenROAD uses the OpenDB database and OpenSTA for static timing analysis.


## Install dependencies

The `etc/DependencyInstaller.sh`  script supports Centos7 and Ubuntu 20.04.
You need root access to correctly install the dependencies with the script.


## Install dependencies

Tools
  * cmake 3.14
  * gcc 8.3.0 or clang7
  * bison 3.0.5
  * flex 2.6.4
  * swig 4.0

Libraries
  * boost 1.68 (1.75 will not compile)
  * tcl 8.6
  * zlibc
  * eigen3
  * spdlog
  * [lemon](http://lemon.cs.elte.hu/pub/sources/lemon-1.3.1.tar.gz)(graph library, not the parser)
  * qt5
  * cimg (optional for replace)


For a limited number of configurations the following script can be used to install
dependencies.
```
./etc/DependencyInstaller.sh -dev
```

## Build

```
git clone --recursive https://github.com/The-OpenROAD-Project/OpenROAD.git
cd OpenROAD
```


### Build by hand
```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

### Build using support script
```
$ ./etc/Build.sh
```

OpenROAD git submodules (cloned by the --recursive flag) are located in `/src`.

The default build type is RELEASE to compile optimized code.
The resulting executable is in `build/src/openroad`.

Optional CMake variables passed as -D<var>=<value> arguments to CMake are show below.

```
CMAKE_BUILD_TYPE DEBUG|RELEASE
CMAKE_CXX_FLAGS - additional compiler flags
TCL_LIB - path to tcl library
TCL_HEADER - path to tcl.h
ZLIB_ROOT - path to zlib
CMAKE_INSTALL_PREFIX
```

### Example with support script
```
$ ./etc/Build.sh --cmake="-DCMAKE_BUILD_TYPE=DEBUG -DTCL_LIB=/path/to/tcl/lib"
```

The default install directory is `/usr/local`.
To install in a different directory with CMake use:

```
cmake .. -DCMAKE_INSTALL_PREFIX=<prefix_path>
```

Alternatively, you can use the `DESTDIR` variable with make.

```
make DESTDIR=<prefix_path> install
```

There are a set of regression tests in `/test`.

```
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

```
> report_flow_metrics gcd_nangate45
                       insts    area util slack_min slack_max  tns_max clk_skew max_slew max_cap max_fanout DPL ANT drv
gcd_nangate45            368     564  8.8     0.112    -0.015     -0.1    0.004        0       0          0   0   0   0
```

#### Run

```
openroad
  -help              show help and exit
  -version           show version and exit
  -no_init           do not read .openroad init file
  -no_splash         do not show the license splash at startup
  -threads count|max number of threads to use
  -exit              exit after reading cmd_file
  cmd_file           source cmd_file
```

OpenROAD sources the Tcl command file `~/.openroad` unless the command
line option `-no_init` is specified.

OpenROAD then sources the command file cmd_file if it is specified on
the command line. Unless the `-exit` command line flag is specified it
enters and interactive Tcl command interpreter.

OpenROAD is run using Tcl scripts. The following commands are used to read
and write design data.

```
read_lef [-tech] [-library] filename
read_def filename
write_def [-version 5.8|5.6|5.5|5.4|5.3] filename
read_verilog filename
write_verilog filename
read_db filename
write_db filename
```

Use the Tcl `source` command to read commands from a file.

```
source [-echo] file
```

If an error is encountered in a command while reading the command file,
the error is printed and no more commands are read from the file. If
`file_continue_on_error` is `1` OpenROAD will continue reading commands
after the error.

If `exit_on_error` is `1` OpenROAD will exit when it encounters an
error.

OpenROAD can be used to make a OpenDB database from LEF/DEF, or
Verilog (flat or hierarchical). Once the database is made it can be
saved as a file with the `write_db` command. OpenROAD can then read
the database with the `read_db` command without reading LEF/DEF or
Verilog.

The `read_lef` and `read_def` commands can be used to build an OpenDB
database as shown below. The `read_lef -tech` flag reads the
technology portion of a LEF file.  The `read_lef -library` flag reads
the MACROs in the LEF file.  If neither of the `-tech` and `-library`
flags are specified they default to `-tech -library` if no technology
has been read and `-library` if a technology exists in the database.

```
read_lef liberty1.lef
read_def reg1.def
# Write the db for future runs.
write_db reg1.db
```

The `read_verilog` command is used to build an OpenDB database as
shown below. Multiple verilog files for a hierarchical design can be
read.  The `link_design` command is used to flatten the design
and make a database.

```
read_lef liberty1.lef
read_verilog reg1.v
link_design top
# Write the db for future runs.
write_db reg1.db
```

#### Example Scripts

Example scripts demonstrating how to run OpenROAD on sample designs
can be found in /test. Flow tests taking sample designs from synthesis
verilog to routed design in the open source technologies Nangate45 and
Sky130 are shown below.

```
gcd_nangate45.tcl
aes_nangate45.tcl
tinyRocket_nangate45.tcl
gcd_sky130.tcl
aes_sky130.tcl
ibex_sky130.tcl
```

Each of these designs use the common script `flow.tcl`.

#### Initialize Floorplan

```
initialize_floorplan
  [-site site_name]               LEF site name for ROWS
  -die_area "lx ly ux uy"         die area in microns
  [-core_area "lx ly ux uy"]      core area in microns
or
  -utilization util               utilization (0-100 percent)
  [-aspect_ratio ratio]           height / width, default 1.0
  [-core_space space
    or "bottom top left right"]   space around core. Should either be one value
                                  for all margins or 4 values for each margin.
                                  default 0.0 (microns)
```

The die area and core size used to write ROWs can be specified
explicitly with the -die_area and -core_area arguments. Alternatively,
the die and core area can be computed from the design size and
utilization as show below:

```
 core_area = design_area / (utilization / 100)
 core_width = sqrt(core_area / aspect_ratio)
 core_height = core_width * aspect_ratio
 core = ( core_space_left, core_space_bottom ) 
        ( core_space_left + core_width, core_space_bottom + core_height )
 die =  ( 0, 0 ) 
        ( core_width + core_space_left + core_space_right, 
          core_height + core_space_bottom + core_space_top )
```

The `initialize_floorplan` command removes existing tracks.
Use the `make_tracks` command to add routing tracks to a floorplan.

```
make_tracks [layer]
            [-x_pitch x_pitch]
            [-y_pitch y_pitch]
            [-x_offset x_offset]
            [-y_offset y_offset]
```

With no arguments `make_tracks` adds X and Y tracks for each routing layer.
With a `-layer` argument `make_tracks` adds X and Y tracks for layer with
options to override the LEF technology X and Y pitch and offset.

Place pins around core boundary.

```
auto_place_pins pin_layer
```

#### Pin placement

Place pins on the boundary of the die on the track grid to
minimize net wire lengths. Pin placement also 
creates a metal shape for each pin using min-area rules.

For designs with unplaced cells, the net wire length is
computed considering the center of the die area as the
unplaced cells' position.

Use the following command to perform pin placement:
```
place_pins [-hor_layers h_layers]  
           [-ver_layers v_layers] 
           [-random_seed seed]
           [-exclude interval]
           [-random]
           [-group_pins pins]
           [-corner_avoidance length]
           [-min_distance distance]
```
- ``-hor_layers`` (mandatory). Specify the layers to create the metal shapes 
of pins placed in horizontal tracks. Can be a single layer or a list of layer names.
- ``-ver_layers`` (mandatory). Specify the layers to create the metal shapes
of pins placed in vertical tracks. Can be a single layer or a list of layer names.
- ``-random_seed``. Specify the seed for random operations.
- ``-exclude``. Specify an interval in one of the four edges of the die boundary
where pins cannot be placed. Can be used multiple times.
- ``-random``. When this flag is enabled, the pin placement is 
random.
- ``-group_pins``. Specify a list of pins to be placed together on the die boundary.
- ``-corner_avoidance distance``. Specify the distance (in micron) from each corner to avoid placing pins.
- ``-min_distance distance``. Specify the minimum distance (in micron) between pins in the die boundary.

The `exclude` option syntax is `-exclude edge:interval`. The `edge` values are
(top|bottom|left|right). The `interval` can be the whole edge, with the `*` value,
or a range of values. Example: `place_pins -hor_layers metal2 -ver_layers metal3 -exclude top:* -exclude right:15-60.5 -exclude left:*-50`.
In the example, three intervals were excluded: the whole top edge, the right edge from 15 microns to 60.5 microns, and the
left edge from the beginning to the 50 microns.

```
place_pin [-pin_name pin_name]
          [-layer layer]
          [-location {x y}]
          [-pin_size {width height}]
```

The `place_pin` command places a specific pin in the specified location, with the specified size.
The `-pin_name` option is the name of a pin of the design.
The `-layer` defines the routing layer where the pin is placed.
The `-location` defines the center of the pin.
The `-pin_size` option defines the width and height of the pin.

```
define_pin_shape_pattern [-layer layer]
                         [-x_step x_step]
                         [-y_step y_step]
                         [-region {llx lly urx ury}]
                         [-size {width height}]
                         [-pin_keepout dist]
```

The `define_pin_shape_pattern` command defines a pin placement grid at the specified layer.
This grid has positions inside the die area, not only at the edges of the die boundary.
The `-layer` option defines a single top most routing layer of the placement grid.
The `-region` option defines the {llx, lly, urx, ury} region of the placement grid.
The `-x_step` and `-y_step` options define the distance between each valid position on the grid.
The `-size` option defines the width and height of the pins assigned to this grid. The center of the
pins are placed on the grid positions. Pins may have half of their shapes outside the defined region.
The `-pin_keepout` option defines the boundary (microns) around existing routing obstructions the pins should avoid, defaults to the `layer` minimum spacing.

```
set_io_pin_constraint -direction direction -pin_names names -region edge:interval
```

The `set_io_pin_constraint` command sets region constraints for pins according the direction or the pin name.
This command can be called multiple times with different constraints. Only one condition should be used for each
command call. The `-direction` argument is the pin direction (input, output, inout, or feedthru).
The `-pin_names` argument is a list of names. The `-region` syntax is the same as the `-exclude` syntax.
To restrict pins to the positions defined with `define_pin_shape_pattern`, use `-region up:{llx lly urx ury}` or `-region up:*`.

```
clear_io_pin_constraints
```

The `clear_io_pin_constraints` command clear all the previous defined constraints and pin shape pattern for top layer placement.

#### Tapcell

Tapcell and endcap insertion.

```
tapcell [-tapcell_master tapcell_master]
        [-endcap_master endcap_master]
        [-distance dist]
        [-halo_width_x halo_x]
        [-halo_width_y halo_y]
        [-tap_nwin2_master tap_nwin2_master]
        [-tap_nwin3_master tap_nwin3_master]
        [-tap_nwout2_master tap_nwout2_master]
        [-tap_nwout3_master tap_nwout3_master]
        [-tap_nwintie_master tap_nwintie_master]
        [-tap_nwouttie_master tap_nwouttie_master]
        [-cnrcap_nwin_master cnrcap_nwin_master]
        [-cnrcap_nwout_master cnrcap_nwout_master]
        [-incnrcap_nwin_master incnrcap_nwin_master]
        [-incnrcap_nwout_master incnrcap_nwout_master]
        [-tap_prefix tap_prefix]
        [-endcap_prefix endcap_prefix]
```
You can find script examples for both 45nm/65nm and 14nm in ```tapcell/etc/scripts```

#### Global Placement

RePlAce global placement.

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
* __bin_grid_count__ : Set bin grid's counts. Default: Defined by internal algorithm. [64,128,256,512,..., int]
* __density__ : Set target density. Default: 0.70 [0-1, float]
* __init_density_penalty__ : Set initial density penalty. Default: 8e-5 [1e-6 - 1e6, float]
* __init_wire_length__coef__ : Set initial wirelength coefficient. Default: 0.25 [unlimited, float] 
* __min_phi_coef__ : Set pcof_min(µ_k Lower Bound). Default: 0.95 [0.95-1.05, float]
* __max_phi_coef__ : Set pcof_max(µ_k Upper Bound). Default: 1.05 [1.00-1.20, float]
* __overflow__ : Set target overflow for termination condition. Default: 0.1 [0-1, float]
* __initial_place_max_iter__ : Set maximum iterations in initial place. Default: 20 [0-, int]
* __initial_place_max_fanout__ : Set net escape condition in initial place when 'fanout >= initial_place_max_fanout'. Default: 200 [1-, int]
* __verbose_level__ : Set verbose level for RePlAce. Default: 1 [0-10, int]

`-timing_driven` does a virtual 'repair_design' to find slacks and
weight nets with low slack.  Use the `set_wire_rc` command to set
resistance and capacitance of estimated wires used for timing.

#### Macro Placement

ParquetFP based macro cell placer. Run `global_placement` before macro placement.
The macro placer places macros/blocks honoring halos, channels and cell row "snapping".

Approximately ceil((#macros/3)^(3/2)) sets corresponding to
quadrisections of the initial placed mixed-size layout are explored and
packed using ParquetFP-based annealing. The best resulting floorplan
according to a heuristic evaluation function kept.

```
macro_placement [-halo {halo_x halo_y}]
                [-channel {channel_x channel_y}]
                [-fence_region {lx ly ux uy}]
                [-snap_layer snap_layer_number]
```

-halo horizontal/vertical halo around macros (microns)
-channel horizontal/vertical channel width between macros (microns)
-fence_region - restrict macro placements to a region (microns). Defaults to the core area.
-snap_layer_number - snap macro origins to this routing layer track

Macros will be placed with max(halo * 2, channel) spacing between macros and the
fence/die boundary. If not solutions are found, try reducing the channel/halo.

#### Detailed Placement

The `detailed_placement` command does detailed placement of instances
to legal locations after global placement.

```
set_placement_padding -global|-instances insts|-masters masters
                      [-left pad_left] [-right pad_right]
detailed_placement [-max_displacement rows]
check_placement [-verbose]
filler_placement [-prefix prefix] filler_masters
optimimize_mirroring
```

The `set_placement_padding` command sets left and right padding in
multiples of the row site width. Use the `set_placement_padding`
command before legalizing placement to leave room for routing. Use the
`-global` flag for padding that applies to all instances. Use the
`instances` argument for instances specific padding.  The instances
can be a list of instance name, or instance object returned by the SDC
`get_cells` command. To specify padding for all instances of a common
master, use the `-filter "ref_name == <name>" option to `get_cells`.

The `set_power_net` command is used to set the power and ground
special net names. The defaults are `VDD` and `VSS`.

The `check_placement` command checks the placement legality. It returns `0` if the
placement is legal.

The `filler_placement` command fills gaps between detail placed instances
to connect the power and ground rails in the rows. `filler_masters` is
a list of master/macro names to use for filling the gaps. Wildcard matching
is supported, so `FILL*` will match `FILLCELL_X1 FILLCELL_X16 FILLCELL_X2 FILLCELL_X32 FILLCELL_X4 FILLCELL_X8`.
To specify a different naming prefix from `FILLER_` use `-prefix <new prefix>`.

The `optimimize_mirroring` command mirrors instances about the Y axis
in vane attempt to minimize the total wire length (hpwl).

#### Gate Resizer

Gate resizer commands are described below.
The resizer commands stop when the design area is `-max_utilization
util` percent of the core area. `util` is between 0 and 100.
The resizer stops and reports and error if the max utilization is exceeded.

```
set_wire_rc [-clock] [-signal]
            [-layer layer_name]
            [-resistance res]
            [-capacitance cap]
```

The `set_wire_rc` command sets the resistance and capacitance used to
estimate delay of routing wires.  Separate values can be specified for
clock and data nets with the `-signal` and `-clock` flags. Without
either `-signal` or `-clock` the resistance and capacitance for clocks
and data nets are set.  Use `-layer` or `-resistance` and
`-capacitance`.  If `-layer` is used, the LEF technology resistance
and area/edge capacitance values for the layer are used for a minimum
width wire on the layer.  The resistance and capacitance values per
length of wire, not per square or per square micron.  The units for
`-resistance` and `-capacitance` are from the first liberty file read,
resistance_unit/distance_unit (typically kohms/micron) and liberty
capacitance_unit/distance_unit (typically pf/micron or ff/micron).  If
distance units are not specified in the liberty file microns are
used.

The `set_layer_rc` command can be used to set the resistance and
capacitance for a layer or via. This is useful if they are missing
from the LEF file or to override the values in the LEF.

```
set_layer_rc [-layer layer]
             [-via via_layer]
             [-capacitance cap]
             [-resistance res]
             [-corner corner]
```

For layers the resistance and capacitance units are the same as
`set_wire_rc` (per length of minimum width wire). `layer` must be the
name of a routing layer.

Via resistance can also be set with the `set_layer_rc` command with the -via keyword.
`-capacitance` is not supported for vias. `via_layer` is the name of a via layer.
Via resistance is per cut/via, not area based.

```
remove_buffers
```

Use the `remove_buffers` command to remove buffers inserted by synthesis. This step is recommended before using `repair_design` so it has more flexibility in buffering nets.

```
estimate_parasitics -placement|-global_routing
```

Estimate RC parasitics based on placed component pin locations. If
there are no component locations no parasitics are added. The
resistance and capacitance are per distance unit of a routing
wire. Use the `set_units` command to check units or `set_cmd_units` to
change units. They should represent "average" routing layer resistance
and capacitance. If the set_wire_rc command is not called before
resizing, the default_wireload model specified in the first liberty
file or with the SDC set_wire_load command is used to make parasitics.  

After the `global_route` command has been called the global routing topology
and layers can be used to estimate parasitics  with the `-global_routing` flag.

```
set_dont_use lib_cells
```

The `set_dont_use` command removes library cells from consideration by
the resizer. `lib_cells` is a list of cells returned by
`get_lib_cells` or a list of cell names (wildcards allowed). For
example, `DLY*` says do not use cells with names that begin with `DLY`
in all libraries.

```
buffer_ports [-inputs]
             [-outputs]
             [-max_utilization util]
```
The `buffer_ports -inputs` command adds a buffer between the input and
its loads.  The `buffer_ports -outputs` adds a buffer between the port
driver and the output port. If  The default behavior is
`-inputs` and `-outputs` if neither is specified.

```
repair_design [-max_wire_length max_length]
              [-max_utilization util]
```

The `repair_design` command inserts buffers on nets to repair max slew, max
capacitance, max fanout violations, and on long wires to reduce RC
delay in the wire. It also resizes gates to normalize slews. 
The resistance/capacitance values in `set_wire_rc` are used to find the
wire delays. Use `-max_wire_length` to specify the maximum length of wires.
The maximum wire length defaults to a value that minimizes the wire delay for the wire
resistance/capacitance values specified by `set_wire_rc`.

Use the `set_max_fanout` SDC command to set the maximum fanout for the design.
```
set_max_fanout <fanout> [current_design]
```

```
repair_tie_fanout [-separation dist]
                  [-verbose]
                  lib_port
```

The `repair_tie_fanout` command connects each tie high/low load to a
copy of the tie high/low cell.  `lib_port` is the tie high/low port,
which can be a library/cell/port name or object returned by
`get_lib_pins`. The tie high/low instance is separated from the load
by `dist` (in liberty units, typically microns).

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
             [-overflow_iterations iterations] \
             [-grid_origin {x y}] \
             [-allow_overflow]

```
Options description:
- **guide_file**: Set the output guides file name (e.g.: -guide_file route.guide")
- **verbose**: Set verbose of report. 0 for less verbose, 1 for medium verbose, 2 for full verbose (e.g.: -verbose *1*)
- **overflow_iterations**: Set the number of iterations to remove the overflow of the routing (e.g.: -overflow_iterations *50*)
- **grid_origin**: Set the origin of the routing grid (e.g.: -grid_origin {1 1})
- **allow_overflow**: Allow global routing results with overflow

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
set_clock_routing [-pdrev_fanout fanout] \
                  [-pdrev_alpha alpha]
```
The `set_clock_routing` command sets specific configurations for clock nets.
Options description:
- **pdrev_fanout**: Set the minimum fanout to use PDRev for the routing topology construction of the clock nets (e.g.: -pdrev_fanout 5)
- **pdrev_alpha**: Set the PDRev routing topology construction trade-off for clock nets. See `set_pdrev_alpha` command description for
more details about PDRev and topology trade-off.
Example: `set_clock_routing -pdrev_fanout 5 -pdrev_alpha 0.6`

```
set_pdrev_alpha [-net net_name] alpha
```
FastRoute has an alternative tool for the routing topology construction, called PDRev. You can define the topology construction
trade-off between minimum wire length and path length between the driver and the loads, using the `alpha` parameter.
The `set_pdrev_alpha` command sets the PDRev routing topology construction trade-off for all nets or for a specific with the `-net` option.
Alpha is between 0.0 and 1.0. When alpha is 0.0 the net topology minimizes total wire length (i.e. capacitance).
When alpha is 1.0 it minimizes longest path between the driver and loads (i.e., maximum resistance).
Typical values are 0.4-0.8. For more information about PDRev, check the paper in `src/FastRoute/src/pdrev/papers/PDRev.pdf`
You can call it multiple times for different nets.
Example: `set_pdrev_alpha -net clk 0.3` sets the alpha value of 0.3 for net *clk*.

```
set_global_routing_region_adjustment {lower_left_x lower_left_y upper_right_x upper_right_y}
                                     -layer layer -adjustment adjustment
```
The `set_global_routing_region_adjustment` command sets routing resources adjustments in a specific region of the design.
The region is defined as a rectangle in a routing layer.
Example: `set_global_routing_region_adjustment {1.5 2 20 30.5}
                                               -layer Metal4 -adjustment 0.7`

```
repair_antennas diodeCellName/diodePinName
```
The repair_antenna command evaluates the global routing results looking for antenna violations, and repairs the violations
by inserting diodes. The input for this command is the diode cell and pin names.
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

```
ord::get_die_area
ord::get_core_area
```

