# OpenROAD

OpenROAD is a chip physical design tool. It uses the OpenDB database
as a design database and representation. OpenSTA is used for static
timing analysis.

#### Build

The OpenROAD build requires the following packages:

  * cmake 3.9
  * gcc or clang
  * bison
  * flex
  * swig 3.0
  * boost
  * tcl 8.5
  * zlib

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

OpenROAD sources the TCL command file `~/.openroad` unless the command
line option `-no_init` is specified.

OpenROAD then sources the command file cmd_file. Unless the `-exit`
command line flag is specified it enters and interactive TCL command
interpreter.

OpenROAD is run using TCL scripts. The following commands are used to read
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

#### Initialize Floorplan

```
initialize_floorplan
  [-site site_name]          LEF site name for ROWS
  [-tracks tracks_file]      routing track specification
  -die_area "lx ly ux uy"    die area in microns
  [-core_area "lx ly ux uy"] core area in microns
or
  -utilization util          utilization (0-100 percent)
  [-aspect_ratio ratio]      height / width, default 1.0
  [-core_space space]        space around core, default 0.0 (microns)
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
 core = ( core_space, core_space ) ( core_space + core_width, core_space + core_height )
 die = ( 0, 0 ) ( core_width + core_space * 2, core_height + core_space * 2 )
```

Place pins around core boundary.

```
auto_place_pins pin_layer
```

#### Gate Resizer

Gate resizer commands are described below.
The resizer commands stop when the design area is `-max_utilization
util` percent of the core area. `util` is between 0 and 100.

```
set_wire_rc [-layer layer_name]
            [-resistance res ]
	    [-capacitance cap]
	    [-corner corner_name]
```
The `set_wire_rc` command sets the resistance and capacitance used to
estimate delay of routing wires.  Use `-layer` or `-resistance` and
`-capacitance`.  If `-layer` is used, the LEF technology resistance
and area/edge capacitance values for the layer are used.  The units
for `-resistance` and `-capacitance` are from the first liberty file
read, resistance_unit/distance_unit and liberty
capacitance_unit/distance_unit. RC parasitics are added based on
placed component pin locations. If there are no component locations no
parasitics are added. The resistance and capacitance are per distance
unit of a routing wire. Use the `set_units` command to check units or
`set_cmd_units` to change units. They should represent "average"
routing layer resistance and capacitance. If the set_wire_rc command
is not called before resizing, the default_wireload model specified in
the first liberty file or with the SDC set_wire_load command is used
to make parasitics.

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
resize [-libraries resize_libraries]
       [-dont_use cells]
       [-max_utilization util]
```
The `resize` command resizes gates to normalize slews.

The `-libraries` option specifies which libraries to use when
resizing. `resize_libraries` defaults to all of the liberty libraries
that have been read. Some designs have multiple libraries with
different transistor thresholds (Vt) and are used to trade off power
and speed. Chosing a low Vt library uses more power but results in a
faster design after the resizing step. Use the `-dont_use` option to
specify a list of patterns of cells to not use. For example, `*/DLY*`
says do not use cells with names that begin with `DLY` in all
libraries.

```
repair_max_cap -buffer_cell buffer_cell
               [-max_utilization util]
repair_max_slew -buffer_cell buffer_cell
                [-max_utilization util]
```
The `repair_max_cap` and `repair_max_slew` commands repair nets with
maximum capacitance or slew violations by inserting buffers in the
net.

```
repair_max_fanout -max_fanout fanout
                  -buffer_cell buffer_cell
                  [-max_utilization util]
```
The `repair_max_fanout` command repairs nets with a fanout greater
than `fanout` by inserting buffers between the driver and the loads.
Buffers are located at the center of each group of loads.

```
repair_tie_fanout [-max_fanout fanout]
                  [-verbose]
                  lib_port
```
The `repair_tie_fanout` command repairs tie high/low nets with fanout
greater than `fanout` by cloning the tie high/low driver.
`lib_port` is the tie high/low port, which can be a library/cell/port
name or object returned by `get_lib_pins`. Clones are located at the
center of each group of loads.

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
read_lef nlc18.lef
read_liberty nlc18.lib
read_def mea.def
read_sdc mea.sdc
set_wire_rc -layer metal2
set buffer_cell [get_lib_cell nlc18_worst/snl_bufx4]
set max_util 90
buffer_ports -buffer_cell $buffer_cell
resize -resize
repair_max_cap -buffer_cell $buffer_cell -max_utilization $max_util
repair_max_slew -buffer_cell $buffer_cell -max_utilization $max_util
# repair tie hi/low before max fanout so they don't get buffered
repair_tie_fanout -max_fanout 100 Nangate/LOGIC1_X1/Z
repair_max_fanout -max_fanout 100 -buffer_cell $buffer_cell -max_utilization $max_util
repair_hold_violations -buffer_cell $buffer_cell -max_utilization $max_util
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
set_placement_padding -global [-left pad_left] [-right pad_right]
detailed_placement [-max_displacement rows]
check_placement [-verbose]
filler_placement filler_masters
set_power_net [-power power_name] [-ground ground_net]
```

The `set_placement_padding` command sets left and right padding in multiples of
the row site width. Use the `set_padding` command before legalizing
placement to leave room for routing.

The `set_power_net` command is used to set the power and ground
special net names. The defaults are `VDD` and `VSS`.

The `check_placement` command checks the placement legality. It returns `1` if the
placement is legal.

The `filler_placement` command fills gaps between detail placed instances
to connect the power and ground rails in the rows. `filler_masters` is
a list of master/macro names to use for filling the gaps. Wildcard matching
is supported, so `FILL*` will match `FILLCELL_X1 FILLCELL_X16 FILLCELL_X2 FILLCELL_X32 FILLCELL_X4 FILLCELL_X8`.

#### Clock Tree Synthesis

Create clock tree subnets.

```
clock_tree_synthesis -lut_file <lut_file> \
                     -sol_list <sol_list_file> \
                     -wire_unit <wire_unit> \
                     -root_buf <root_buf> \
                     [-clk_nets <list_of_clk_nets>]
```
- ```lut_file```, ```sol_list``` and ```wire_unit``` are parameters related to the technology characterization described [here](https://github.com/The-OpenROAD-Project/TritonCTS/blob/master/doc/Technology_characterization.md).
- ``root_buffer`` is the master cell of the buffer that serves as root for the clock tree.
- ``clk_nets`` is a string containing the names of the clock roots. If this parameter is ommitted, TritonCTS looks for the clock roots automatically.

#### Global Routing

FastRoute global route.
Generate routing guides given a placed design.

```
fastroute -output_file out_file
          -capacity_adjustment <cap_adjust>
          -min_routing_layer <min_layer>
          -max_routing_layer <max_layer>
          -pitches_in_tile <pitches>
          -layers_adjustments <list_of_layers_to_adjust>
          -regions_adjustments <list_of_regions_to_adjust>
          -nets_alphas_priorities <list_of_alphas_per_net>
          -verbose <verbose>
          -unidirectional_routing
          -clock_net_routing
```

Options description:
- **capacity_adjustment**: Set global capacity adjustment (e.g.: -capacity_adjustment *0.3*)
- **min_routing_layer**: Set minimum routing layer (e.g.: -min_routing_layer *2*)
- **max_routing_layer**: Set maximum routing layer (e.g.: max_routing_layer *9*)
- **pitches_in_tile**: Set the number of pitches inside a GCell
- **layers_adjustments**: Set capacity adjustment to specific layers (e.g.: -layers_adjustments {{<layer> <reductionPercentage>} ...})
- **regions_adjustments**: Set capacity adjustment to specific regions (e.g.: -regions_adjustments {{<minX> <minY> <maxX> <maxY> <layer> <reductionPercentage>} ...})
- **nets_alphas_priorities**: Set alphas for specific nets when using clock net routing (e.g.: -nets_alphas_priorities {{<net_name> <alpha>} ...})
- **verbose**: Set verbose of report. 0 for less verbose, 1 for medium verbose, 2 for full verbose (e.g.: -verbose 1)
- **unidirectional_routing**: Activate unidirectional routing *(flag)*
- **clock_net_routing**: Activate clock net routing *(flag)*

###### NOTE 1: if you use the flag *unidirectional_routing*, the minimum routing layer will be assigned as "2" automatically
###### NOTE 2: the first routing layer of the design have index equal to 1
###### NOTE 3: if you use the flag *clock_net_routing*, only guides for clock nets will be generated


#### Logical and Physical Optimizations

OpenPhySyn Perform additional timing and area optimization.

```
set_psn_wire_rc [-layer layer_name]
            [-resistance res_per_micron ]
      [-capacitance cap_per_micron]
```
The `set_psn_wire_rc` command sets the average wire resistance/capacitance per micron; you can use -layer <layer_name> only to extract the value from the LEF technology. It should be invoked before physical optimization commands.

```
optimize_logic
        [-tiehi tiehi_cell_name] 
        [-tielo tielo_cell_name] 
```
The `optimize_logic` command should be run after the logic synthesis on hierarical designs to perform logic optimization; currently, it performs constant propagation to reduce the design area. You can optionally specify the name of tie-hi/tie-lo liberty cell names to use for the optimization.

```
optimize_design
        [-no_gate_clone]
        [-no_pin_swap]
        [-clone_max_cap_factor factor]
        [-clone_non_largest_cells]
```
The `optimize_design` command can be used for additional timing optimization, it should be run after the global placmenet. Currently it peforms gate cloning and comuttaitve pin swapping to enhance the timing.

```
optimize_fanout
        -buffer_cell buffer_cell_name
        -max_fanout max_fanout
```
The `optimize_fanout` command can be run after the logical synthesis to perform basic buffering based on the number of fanout pins.


#### PDN analysis

PDNSim IR analysis.
Report worst IR drop given a placed and PDN synthesized design

```
analyze_power_grid -vsrc <voltage_source_location_file>
```

Options description:
- **vsrc**: Set the location of the power C4 bumps/IO pins

###### Note: See the file [Vsrc_aes.loc file](https://github.com/The-OpenROAD-Project/PDNSim/blob/master/test/aes/Vsrc.loc) for an example with a description specified [here](https://github.com/The-OpenROAD-Project/PDNSim/blob/master/doc/Vsrc_description.md).
