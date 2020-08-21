# OpenROAD

OpenROAD is a chip physical design tool. It uses the OpenDB database
as a design database and representation. OpenSTA is used for static
timing analysis.

#### Build

The OpenROAD build requires the following packages:

Tools
  * cmake 3.14
  * gcc 8.3.0 or clang
  * bison 3.0.5
  * flex 2.6.4
  * swig 4.0

Libraries
  * boost 1.68
  * tcl 8.6
  * zlib
  * eigen
  * lemon
  * qt5
  * CImg (optional for replace)


See `Dockerfile` for an example of how to install these packages. 

```
git clone --recursive https://github.com/The-OpenROAD-Project/OpenROAD.git
cd OpenROAD
mkdir build
cd build
cmake ..
make
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
test/regression
src/resizer/test/regression

```

#### Run

```
openroad
  -help              show help and exit
  -version           show version and exit
  -no_init           do not read .openroad init file
  -no_splash         do not show the license splash at startup
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
  [-tracks tracks_file]           routing track specification
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

If no -tracks file is used the routing layers from the LEF are used.

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

Place pins around core boundary.

```
auto_place_pins pin_layer
```

#### I/O pin assignment

Assign I/O pins to on-track locations at the boundaries of the 
core while optimizing I/O nets wirelength. I/O pin assignment also 
creates a metal shape for each I/O pin using min-area rules.

Use the following command to perform I/O pin assignment:
```
place_ios [-hor_layer h_layer]  
          [-ver_layer v_layer] 
	  [-random_seed seed] 
          [-random] 
```
- ``-hor_layer`` (mandatory). Set the layer to create the metal shapes 
of I/O pins assigned to horizontal tracks. 
- ``-ver_layer`` (mandatory). Set the layer to create the metal shapes
of I/O pins assigned to vertical tracks. 
- ``-random_seed``. Set the seed for random operations.
- ``-random``. When this flag is enabled, the I/O pin assignment is 
random.

#### Gate Resizer

Gate resizer commands are described below.
The resizer commands stop when the design area is `-max_utilization
util` percent of the core area. `util` is between 0 and 100.

```
set_wire_rc [-clock] [-data]
	    [-layer layer_name]
            [-resistance res ]
	    [-capacitance cap]
	    [-corner corner_name]
```

The `set_wire_rc` command sets the resistance and capacitance used to
estimate delay of routing wires.  Separate values can be specified for
clock and data nets with the `-data` and `-clock` flags. Without
either `-data` or `-clock` the resistance and capacitance for clocks
and data nets are set.  Use `-layer` or `-resistance` and
`-capacitance`.  If `-layer` is used, the LEF technology resistance
and area/edge capacitance values for the layer are used for a minimum
width wire on the layer.  The resistance and capacitance values per
length of wire, not per square or per square micron.  The units for
`-resistance` and `-capacitance` are from the first liberty file read,
resistance_unit/distance_unit (typically kohms/micron) and liberty
capacitance_unit/distance_unit (typically pf/micron or ff/micron).  If
no distance units are not specied in the liberty file microns are
used.

```
estimate_parasitics -placement
```

Estimate RC parasitics based on placed component pin locations. If
there are no component locations no parasitics are added. The
resistance and capacitance are per distance unit of a routing
wire. Use the `set_units` command to check units or `set_cmd_units` to
change units. They should represent "average" routing layer resistance
and capacitance. If the set_wire_rc command is not called before
resizing, the default_wireload model specified in the first liberty
file or with the SDC set_wire_load command is used to make parasitics.

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
	     -buffer_cell buffer_cell
```
The `buffer_ports -inputs` command adds a buffer between the input and
its loads.  The `buffer_ports -outputs` adds a buffer between the port
driver and the output port. If  The default behavior is
`-inputs` and `-outputs` if neither is specified.

```
repair_design [-max_wire_length max_length]
              -buffer_cell buffer_cell
```

The `repair_design` inserts buffers on nets to repair max slew, max
capacitance, max fanout violations, and on long wires to reduce RC
delay in the wire. Use `-max_wire_length` to specify the maximum lenth
of wires.  The resistance/capacitance values in `set_wire_rc` are used
to find the wire delays.

Use the `set_max_fanout` SDC command to set the maximum fanout for the design.
```
set_max_fanout <fanout> [current_design]
```

```
resize [-libraries resize_libraries]
       [-max_utilization util]
```
The `resize` command resizes gates to normalize slews.

The `-libraries` option specifies which libraries to use when
resizing. `resize_libraries` defaults to all of the liberty libraries
that have been read. Some designs have multiple libraries with
different transistor thresholds (Vt) and are used to trade off power
and speed. Chosing a low Vt library uses more power but results in a
faster design after the resizing step.

```
repair_tie_fanout [-separation dist]
                  [-verbose]
                  lib_port
```

The `repair_tie_fanout` command connects each tie high/low load to a
copy of the tie high/low cell.  `lib_port` is the tie high/low port,
which can be a library/cell/port name or object returned by
`get_lib_pins`. The tie high/low instance is separaated from the load
by `dist` (in liberty units, typically microns).

```
repair_hold_violations -buffer_cell buffer_cell
                       [-max_utilization util]
```
The `repair_hold_violations` command inserts buffers to repair hold
check violations.

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

A typical resizer command file is shown below.

```
# resizer/test/gcd_resize.tcl
read_liberty Nangate_typ.lib
read_lef Nangate.lef
read_def gcd_placed.def
read_sdc gcd.sdc

set_wire_rc -layer metal2

set buffer_cell BUF_X4
set_dont_use {CLKBUF_* AOI211_X1 OAI211_X1}

buffer_ports -buffer_cell $buffer_cell
repair_design -max_wire_length 100 -buffer_cell $buffer_cell
resize
repair_tie_fanout LOGIC0_X1/Z
repair_tie_fanout LOGIC1_X1/Z
repair_hold_violations -buffer_cell $buffer_cell
resize
```

Note that OpenSTA commands can be used to report timing metrics before
or after resizing the design.

```
set_wire_rc -layer metal2
report_checks
report_tns
report_wns
report_checks

resize

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

#### Tapcell

Tapcell and endcap insertion.

```
tapcell -tapcell_master <tapcell_master>
        -endcap_master <endcap_master>
        -endcap_cpp <endcap_cpp>
        -distance <dist>
        -halo_width_x <halo_x>
        -halo_width_y <halo_y>
        -tap_nwin2_master <tap_nwin2_master>
        -tap_nwin3_master <tap_nwin3_master>
        -tap_nwout2_master <tap_nwout2_master>
        -tap_nwout3_master <tap_nwout3_master>
        -tap_nwintie_master <tap_nwintie_master>
        -tap_nwouttie_master <tap_nwouttie_master>
        -cnrcap_nwin_master <cnrcap_nwin_master>
        -cnrcap_nwout_master <cnrcap_nwout_master>
        -incnrcap_nwin_master <incnrcap_nwin_master>
        -incnrcap_nwout_master <incnrcap_nwout_master>
        -tbtie_cpp <tbtie_cpp>
        -no_cell_at_top_bottom
        -add_boundary_cell
```
You can find script examples for both 45nm/65nm and 14nm in ```tapcell/etc/scripts```

#### Global Placement

RePlAce global placement.

```
global_placement [-timing_driven]
                 [-bin_grid_count grid_count]
```
- **timing_driven**: Enable timing-driven mode
- **grid_count**: [64,128,256,512,..., int]. Default: Defined by internal algorithm.

Use the `set_wire_rc` command to set resistance and capacitance of
estimated wires used for timing.

#### Detailed Placement

The `detailed_placement` command does detailed placement of instances
to legal locations after global placement.

```
set_placement_padding -global|-instances insts|-masters masters
                      [-left pad_left] [-right pad_right]
detailed_placement [-max_displacement rows]
check_placement [-verbose]
filler_placement filler_masters
set_power_net [-power power_name] [-ground ground_net]
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

The `check_placement` command checks the placement legality. It returns `1` if the
placement is legal.

The `filler_placement` command fills gaps between detail placed instances
to connect the power and ground rails in the rows. `filler_masters` is
a list of master/macro names to use for filling the gaps. Wildcard matching
is supported, so `FILL*` will match `FILLCELL_X1 FILLCELL_X16 FILLCELL_X2 FILLCELL_X32 FILLCELL_X4 FILLCELL_X8`.

The `optimimize_mirroring` command mirrors instances about the Y axis
in vane attempt to minimize the total wire length (hpwl).

#### Clock Tree Synthesis

Create clock tree subnets. There are currently two ways one can run this command.
The first is if the user does not have a characterization file. Thus, the wire segments are created manually based on the user parameters. 

```
clock_tree_synthesis -buf_list <list_of_buffers> \
                     -sqr_cap <cap_per_sqr> \
                     -sqr_res <res_per_sqr> \
                     [-root_buf <root_buf>] \
                     [-max_slew <max_slew>] \
                     [-max_cap <max_cap>] \
                     [-slew_inter <slew_inter>] \
                     [-cap_inter <cap_inter>] \
                     [-wire_unit <wire_unit>] \
                     [-clk_nets <list_of_clk_nets>] \
                     [-out_path <lut_path>] \
                     [-characterization_only]
```

- ``-buf_list`` are the master cells (buffers) that will be considered when making the wire segments.
- ``-sqr_cap`` is the capacitance (in picofarad) per micrometer (thus, the same unit that is used in the LEF syntax) to be used in the wire segments. 
- ``-sqr_res`` is the resistance (in ohm) per micrometer (thus, the same unit that is used in the LEF syntax) to be used in the wire segments. 
- ``-root_buffer`` is the master cell of the buffer that serves as root for the clock tree. 
If this parameter is omitted, the first master cell from ``-buf_list`` is taken.
- ``-max_slew`` is the max slew value (in seconds) that the characterization will test. 
If this parameter is omitted, the code tries to obtain the value from the liberty file.
- ``-max_cap`` is the max capacitance value (in farad) that the characterization will test. 
If this parameter is omitted, the code tries to obtain the value from the liberty file.
- ``-slew_inter`` is the time value (in seconds) that the characterization will consider for results. 
If this parameter is omitted, the code gets the default value (5.0e-12). Be careful that this value can be quite low for bigger technologies (>65nm).
- ``-cap_inter`` is the capacitance value (in farad) that the characterization will consider for results. 
If this parameter is omitted, the code gets the default value (5.0e-15). Be careful that this value can be quite low for bigger technologies (>65nm).
- ``-wire_unit`` is the minimum unit distance between buffers for a specific wire. 
If this parameter is omitted, the code gets the value from ten times the height of ``-root_buffer``.
- ``-clk_nets`` is a string containing the names of the clock roots. 
If this parameter is omitted, TritonCTS looks for the clock roots automatically.
- ``-out_path`` is the output path (full) that the lut.txt and sol_list.txt files will be saved. This is used to load an existing characterization, without creating one from scratch.
- ``-only_characterization`` is a flag that, when specified, makes so that only the library characterization step is run and no clock tree is inserted in the design.

Instead of creating a characterization, you can use use the following parameters to load a characterization file.

```
clock_tree_synthesis -lut_file <lut_file> \
                     -sol_list <sol_list_file> \
                     -root_buf <root_buf> \
                     [-wire_unit <wire_unit>] \
                     [-clk_nets <list_of_clk_nets>] 
```

- ``-lut_file`` (mandatory) is the file containing delay, power and other metrics for each segment.
- ``-sol_list`` (mandatory) is the file containing the information on the topology of each segment (wirelengths and buffer masters).
- ``-sqr_res`` (mandatory) is the resistance (in ohm) per database units to be used in the wire segments. 
- ``-root_buffer`` (mandatory) is the master cell of the buffer that serves as root for the clock tree. 
If this parameter is omitted, you can use the ``-buf_list`` argument, using the first master cell. If both arguments are omitted, an error is raised.
- ``-wire_unit`` (optional) is the minimum unit distance between buffers for a specific wire, based on your ``-lut_file``. 
If this parameter is omitted, the code gets the value from the header of the ``-lut_file``. For the old technology characterization, described [here](https://github.com/The-OpenROAD-Project/TritonCTS/blob/master/doc/Technology_characterization.md), this argument is mandatory, and omitting it raises an error.
- ``-clk_nets`` (optional) is a string containing the names of the clock roots. 
If this parameter is omitted, TritonCTS looks for the clock roots automatically.

Another command available from TritonCTS is ``report_cts``. It is used to extract metrics after a successful ``clock_tree_synthesis`` run. These are: Number of Clock Roots, Number of Buffers Inserted, Number of Clock Subnets, and Number of Sinks.

```
read_lef "mylef.lef"
read_liberty "myliberty.lib"
read_def "mydef.def"
read_verilog "myverilog.v"
read_sdc "mysdc.sdc"

report_checks

clock_tree_synthesis -lut_file "lut.txt" \
                     -sol_list "sol_list.txt" \
                     -root_buf "BUF_X4" \
                     -wire_unit 20 

report_cts [-out_file "file.txt"]
```

- ``-out_file`` (optional) is the file containing the TritonCTS reports.
If this parameter is omitted, the metrics are shown on the standard output.


#### Global Routing

Global router options and commands are described below. 

```
fastroute -capacity_adjustment <cap_adjust>
          -min_routing_layer <min_layer>
          -max_routing_layer <max_layer>
          -tile_size <tile_size>
          -verbose <verbose>
          -overflow_iterations <iterations>
          -grid_origin {x y}
          -report_congestion congest_file
          -unidirectional_routing
          -allow_overflow

```

Options description:
- **capacity_adjustment**: Set global capacity adjustment (e.g.: -capacity_adjustment *0.3*)
- **min_routing_layer**: Set minimum routing layer (e.g.: -min_routing_layer *2*)
- **max_routing_layer**: Set maximum routing layer (e.g.: -max_routing_layer *9*)
- **tile_size**: Set the number of pitches inside a GCell (e.g.: -tile_size *20*)
- **verbose**: Set verbose of report. 0 for less verbose, 1 for medium verbose, 2 for full verbose (e.g.: -verbose *1*)
- **overflow_iterations**: Set the number of iterations to remove the overflow of the routing (e.g.: -overflow_iterations *50*)
- **grid_origin**: Set the origin of the routing grid (e.g.: -grid_origin {1 1})
- **report_congestion**: Create a text file with the congestion report of the GCells (e.g.: -report_congestion "congest")
- **unidirectional_routing**: Avoid routing in layer 1, using it only for pin access
- **allow_overflow**: Allow global routing results with overflow

```
set_global_routing_layer_adjustment -layer <layer> -adjustment <adjustment>
```

The `set_global_routing_layer_adjustment` command sets routing resources adjustments in a specific routing layer.
You can call it multiple times
Example: `set_global_routing_layer_adjustment -layer 4 -adjustment 0.5` reduces the routing resources of routing layer 4 in 50%

```
set_global_routing_layer_pitch -layer <layer> -pitch <pitch>
```

The `set_global_routing_layer_pitch` command sets the pitch for routing tracks in a specific layer
Example: `set_global_routing_layer_pitch -layer 6 -pitch 1.34`

```
set_pdrev_topology_priority -net <netName> -alpha <alpha>
```

The `set_pdrev_topology_priority` command sets the PDRev Steiner tree construction priority between wirelength and skew.
Alpha is a positive float between 0.0 and 1.0, where alpha close to 0.0 generates better wirelength, and alpha close to 1.0 generates better skew.
Example: `set_pdrev_topology_priority -net "clk" -alpha 0.3` sets an alpha value of 0.3 for net *clk*

```
set_clock_route_flow -min_layer <min_layer> -max_layer <max_layer>
                     -pdrev_for_high_fanout <fanout> -alpha <alpha>
```

The `set_clock_route_flow` command enable a special route flow, where you can define min and max routing layer for clock nets, a minimum fanout
to use PDRev for Stener tree construction, and the alpha value for PDRev
Example: `set_clock_route_flow -min_layer 4 -max_layer 8  -pdrev_for_high_fanout 5 -alpha 0.4` will route clock nets using the layer range 4 to 8. Lower layers are used for pin access.
Nets with 5 pins or more have Steiner trees generated by PDRev, with an alpha value equal 0.4

```
repair_antenna -diode_cell_name <cell_name> -diode_pin_name <pin_name>
```

The `repair_antenna` command evaluates the global routing results looking for antenna violations, and repairs the violations with a diode insertion process.
You need to define the diode cell and pin names
Example: `repair_antenna -diode_cell_name "diode_cell" -diode_pin_name "diode_pin"`

```
write_guides <file_name>
```

The `write_guides` generates the guide file from the routing results
Example: `write_guides route.guide`

**NOTE 1**: the first routing layer of the design have index equal to 1

**NOTE 2**: commands `set_global_routing_layer_adjustment`, `set_global_routing_layer_pitch`, `set_pdrev_topology_priority` and `set_clock_route_flow` should be
called before running `fastroute`. They are configuration commands

**NOTE 3**: commands `repair_antenna` and `write_guides` should be called after running `fastroute`.

To estimate RC parasitics based on global route results, use the `-global_routing`
option of the `estimate_parasitics` command.

```
estimate_parasitics -global_routing
```

#### Logical and Physical Optimizations

OpenPhySyn performs additional timing and area optimization.


```
optimize_logic
        [-tiehi tiehi_cell_name] 
        [-tielo tielo_cell_name] 
```
The `optimize_logic` command should be run after the logic synthesis on hierarical designs to perform logic optimization; currently, it performs constant propagation to reduce the design area. You can optionally specify the name of tie-hi/tie-lo liberty cell names to use for the optimization.


```
repair_timing
        [-capacitance_violations]
        [-transition_violations]
        [-negative_slack_violations]
        [-iterations iteration_count]
        [-buffers buffer_cells]
        [-inverters inverter cells]
        [-auto_buffer_library <single|small|medium|large|all>]
        [-no_minimize_buffer_library]
        [-auto_buffer_library_inverters_enabled]
        [-buffer_disabled]
        [-resize_disabled]
        [-pin_swap_disabled]
        [-capacitance_pessimism_factor factor]
        [-transition_pessimism_factor factor]
        [-minimum_cost_buffer_enabled]
        [-legalization_frequency num_edits]
        [-legalize_each_iteration iterations]
        [-legalize_eventually]
        [-post_place]
        [-post_route]
        [-minimum_gain gain]
        [-high_effort]
        [-maximum_negative_slack_paths count]
        [-maximum_negative_slack_path_depth count]
        [-pins pin_names]
```
The `repair_timing` command repairs negative slack, maximum capacitance and transition violations by buffer tree insertion, gate sizing, and pin-swapping.

`repair_timing` options:
-   `[-capacitance_violations]`: Repair capacitance violations.
-   `[-transition_violations]`: Repair transition violations.
-   `[-negative_slack_violations]`: Repair paths with negative slacks.
-   `[-iterations iterations]`: Maximum number of iterations.
-   `[-buffers buffer_cells]`: Manually specify buffer cells to use.
-   `[-inverters inverter cells]`: Manually specify inverter cells to use.
-   `[-auto_buffer_library <single|small|medium|large|all>]`: Auto-select buffer library.
-   `[-no_minimize_buffer_library]`: Do not run initial pruning phase for buffer selection.
-   `[-auto_buffer_library_inverters_enabled]`: Include inverters in the selected buffer library.
-   `[-buffer_disabled]`: Disable all buffering.
-   `[-resize_disabled]`: Disable driver sizing.
-   `[-pin_swap_disabled]`: Disable pin-swapping.
-   `[-transition_pessimism_factor factor]` Scaling factor for transition violation limits, default is 1.0, should be non-negative, < 1.0 is pessimistic, 1.0 is ideal, > 1.0 is optimistic (default is 1.0).
-   `[-capacitance_pessimism_factor factor]` Scaling factor for capacitance violation limits, default is 1.0, should be non-negative, < 1.0 is pessimistic, 1.0 is ideal, > 1.0 is optimistic (default is 1.0).
-   `[-minimum_cost_buffer_enabled]`: Enable minimum cost buffering.
-   `[-legalization_frequency <num_edits>]`: Legalize after how many edits.
-   `[-legalize_eventually]`: Legalize at the end of the optimization.
-   `[-legalize_each_iteration]`: Legalize after each iteration.
-   `[-post_place|-post_route]`: Post-placement phase mode or post-routing phase mode (not currently supported).
-   `[-minimum_gain <unit_time>]`: Minimum slack gain to accept an optimization.
-   `[-high_effort]`: Trade-off runtime versus optimization quality by weaker pruning.
-   `[-no_resize_for_negative_slack]`: Disable resizing when solving negative slack violations (enhances runtime).
-   `[-maximum_negative_slack_paths count]`: Maximum number of negative slack paths to try to optimize.
-   `[-maximum_negative_slack_path_depth count]`: Maximum depth per negative slack path to try to optimize.
-   `[-pins pin_names]`: Manually select the pins to optimize.

```
pin_swap
       -path_count count
       [-power]
```
The `pin_swap` command performs additional timing optimization by commutative pin swapping.


```
optimize_fanout
        -buffer_cell buffer_cell_name
        -max_fanout max_fanout
```
The `optimize_fanout` command can be run after the logical synthesis to perform basic buffering based on the number of fanout pins.


```
cluster_buffers
  [-cluster_threshold diameter]
  [-cluster_size single|small|medium|large|all]
```
The `cluster_buffers` command can be used to automatically select representative set of buffers through k-center clustering.





#### PDN analysis

PDNSim PDN checker searches for floating PDN stripes.

PDNSim reports worst IR drop given a placed and PDN synthesized design.

```
check_power_grid -net <VDD/VSS>
analyze_power_grid -vsrc <voltage_source_location_file>
write_pg_spice -vsrc <voltage_source_location_file> -outfile <netlist.sp>
```

Options description:
- **vsrc**: Set the location of the power C4 bumps/IO pins

###### Note: See the file [Vsrc_aes.loc file](https://github.com/The-OpenROAD-Project/PDNSim/blob/master/test/aes/Vsrc.loc) for an example with a description specified [here](https://github.com/The-OpenROAD-Project/PDNSim/blob/master/doc/Vsrc_description.md).
