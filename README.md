# OpenROAD

OpenROAD is a chip physical design tool. It uses the OpenDB database
as a design database and representation. OpenSTA is used for static
timing analysis.

#### Build

OpenROAD depends on OpenSTA, and OpenDB, and flute3. These source
directories are git submodules and located in `/src`.

```
git clone --recursive https://github.com/The-OpenROAD-Project/OpenROAD.git
cd OpenROAD
mkdir build
cd build
cmake ..
make
```

The default build type is RELEASE to compile optimized code.
The resulting executable is in `build/resizer`.

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
test/regression fast
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

OpenROAD is run using TCL scripts.  In addition to the OpenSTA
commands documented in OpenSTA/doc/OpenSTA.pdf, available commands are
shown below.

```
read_lef [-tech] [-library] filename
read_def filename
write_def filename
read_verilog filename
write_verilog filename
read_db filename
write_db filename
initialize_floorplan 
```

OpenROAD can be used to make and OpenDB database from LEF/DEF, or
Verilog (flat or hierarchical). Once the database is made it can be
saved as a file with the `write_db` command. OpenROAD can then read
the database with the `read_db` command without reading LEF/DEF or
Verilog.

The `read_lef` and `read_def` commands can be used to build an OpenDB
database as shown below. The `read_lef -tech` flag reads the
technology portion of a LEF file.  The `read_lef -library` flag reads
the MACROs in the LEF file.  If neither of the `-tech` and `-library`
flags are specified they default to `-tech -library` if no technology
has been read and `-library` if a technogy exists in the database.

```
read_lef liberty1.lef
read_def reg1.def
# Write the db for future runs.
write_db reg1.def
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

initialize_floorplan
  [-site site_name]          LEF site name for ROWS
  [-tracks tracks_file]      routing track specification
  -die_area "lx ly ux uy"    die area in microns
  [-core_area "lx ly ux uy"] core area in microns
or
  -utilization util          utilization (0-100 percent)
  [-aspect_ratio ratio]      height / width, default 1.0
  [-core_space space]        space around core, default 0.0 (microns)

The die area and core size used to write ROWs can be specified
explicitly with the -die_area and -core_area arguments. Alternatively,
the die and core area can be computed from the design size and
utilization as show below:

 core_area = design_area / (utilization / 100)
 core_width = sqrt(core_area / aspect_ratio)
 core_height = core_width * aspect_ratio
 core = ( core_space, core_space ) ( core_space + core_width, core_space + core_height )
 die = ( 0, 0 ) ( core_width + core_space * 2, core_height + core_space * 2 )

Place pins around core boundary.

  auto_place_pins pin_layer

#### Gate Resizer

Gate resizer commands are shown below.

```
set_wire_rc [-layer layer_name] [-resistance res ] [-capacitance cap] [-corner corner_name]
resize [-buffer_inputs]
       [-buffer_outputs]
       [-resize]
       [-resize_libraries resize_libraries]
       [-repair_max_cap]
       [-repair_max_slew]
       [-buffer_cell buffer_cell]
       [-dont_use cells]
       [-max_utilization util]
report_design_area
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

The `resize` command buffers inputs and outputs, resizes gates, and
then uses buffer insertion to repair maximum capacitance and slew
violations. Use the `-buffer_inputs`, `-buffer_outputs`, `-resize`,
`-repair_max_cap` and `-repair_max_slew` options to invoke a single
mode. With none of the options specified all are done. The
`-buffer_cell` argument is required for buffer insertion
(`-repair_max_cap` or `-repair_max_slew`). The `-resize_libraries`
option specifies which libraries to use when
resizing. `resize_libraries` defaults to all of the liberty libraries
that have been read. Some designs have multiple libraries with
different transistor thresholds (Vt) and are used to trade off power
and speed. Chosing a low Vt library uses more power but results in a
faster design after the resizing step. Use the `-dont_use` keyword to
specify a list of patterns of cells to not use. For example, "*/DLY*"
says do not use cells with names that begin with "DLY" in all
libraries.

The resizer stops when the design area is `-max_utilization util`
percent of the core area. `util` is between 0 and 100.

A typical resizer command file is shown below.

```
read_lef nlc18.lef
read_liberty nlc18.lib
read_def mea.def
read_sdc mea.sdc
set_wire_rc -resistance 1.67e+05 -capacitance 1.33e-10
set_design_size -die "0 0 1000 1000" -core "100 100 900 900"
resize -buffer_cell [get_lib_cell nlc18_worst/snl_bufx4] -max_utilization 90
```

Note that OpenSTA commands can be used to report timing metrics before
or after resizing the design.

```
set_wire_rc -resistance 1.67e+05 -capacitance 1.33e-10
report_checks
report_tns
report_wns
report_checks

resize

report_checks
report_tns
report_wns
```

The report_design_area command reports the area of the design's
components and the utilization.

#### Timing Analysis

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

#### Global Placement

RePlAce global placement.

```
global_placement
    [-timing_driven]
    [-bin_grid_count grid_count]
```

-timing_driven Enable timing-driven mode
grid_count [64,128,256,512,..., int]. Default: Defined by internal algorithm.

Use the `set_wire_rc` command to set resistance and capacitance of
estimated wires used for timing.

#### Detailed Placement

Legalize a design that has been globally placed.

```
legalize_placement [-constraints constraints_file]
```

## Authors

* James Cherry (OpenSTA, Resizer, OpenROAD)
* Lukas van Ginneken (Resizer algorithm)
* Chris Chu (Flute steiner tree package)
