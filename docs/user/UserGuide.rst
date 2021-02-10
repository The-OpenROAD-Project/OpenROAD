User Guide
==========

OpenROAD is divided into a number of tools that are orchestrated
together to achieve RTL-to-GDS. As of the current implementation, the
flow is divided into four stages:

1. Logic Synthesis: is performed by
   `yosys <https://github.com/The-OpenROAD-Project/yosys>`__.
2. Floorplanning through Detailed Routing: are performed by `OpenROAD
   App <https://github.com/The-OpenROAD-Project/OpenROAD>`__.
3. KLayout: GDS merge, DRC and LVS (public PDKs)

To Run OpenROAD flow, we provide scripts to automate the RTL-to-GDS
stages. Alternatively, you can run the individual steps manually.

[OPTION 1] RTL-to-GDS Flow
--------------------------

**GitHub:**
`OpenROAD-flow-public <https://github.com/The-OpenROAD-Project/OpenROAD-flow-public>`__

Code Organization
~~~~~~~~~~~~~~~~~

This repository serves as an example RTL-to-GDS flow using the OpenROAD
tools.

The two main components are:

1. **tools**: This directory contains the source code for the entire
   ``openroad`` app (via submodules) as well as other tools required for
   the flow. The script ``build_openroad.sh`` in this repository will
   automatically build the OpenROAD toolchain.

2. **flow**: This directory contains reference recipes and scripts to
   run \| designs through the flow. It also contains platforms and test
   designs.

Setup
~~~~~

The flow has the following dependencies:

-  OpenROAD
-  KLayout
-  Yosys

The dependencies can either be obtained from a pre-compiled build export
or built manually. See the `KLayout website <https://www.klayout.de/>`__
for installation instructions.

Option 1: Installing build exports*\*
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Clone the OpenROAD-flow repository

   ::

      git clone --recursive https://github.com/The-OpenROAD-Project/OpenROAD-flow.git

2. Navigate to the “Releases” tab and download the latest release

3. Extract the tar to ``OpenROAD-flow/tools/OpenROAD``

4. Update your shell environment

   ::

      source setup_env.sh

Option 2: Building the tools using docker*\*
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This build option leverages a multi-step docker flow to install the
tools and dependencies to a runner image. To follow these instructions,
you must have docker installed, permissions to run docker, and docker
container network access enabled. This step will create a runner image
tagged as ``openroad/flow``.

1. Clone the OpenROAD-flow repository

   ::

      git clone --recursive https://github.com/The-OpenROAD-Project/OpenROAD-flow.git

2. Ensure your docker daemon is running and ``docker`` is in your PATH,
   then run the docker build.

   ::

      ./build_openroad.sh

3. Start an interactive shell in a docker container using your user
   credentials

   ::

      docker run -u $(id -u ${USER}):$(id -g ${USER}) openroad/flow bash

Option 3: Building the tools locally*\*
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Reference the Dockerfiles and READMEs for the separate tools on the
   build steps and dependencies.

   ::

      OpenROAD-flow/tools/OpenROAD/Dockerfile
      OpenROAD-flow/tools/yosys/Dockerfile

See the `KLayout <https://www.klayout.de/>`__ instructions for
installing KLayout from source.

2. Run the build script

   ::

      ./build_openroad.sh

3. Update your shell environment

   ::

      source setup_env.sh

   ``klayout`` must be added to the path manually.

Using the flow
~~~~~~~~~~~~~~

See the flow
`README <https://github.com/The-OpenROAD-Project/OpenROAD-flow-public/blob/master/flow/README.md>`__
for details about the flow and how to run designs through the flow.

[OPTION 2] Individual Flow Steps
--------------------------------

Logic Synthesis
~~~~~~~~~~~~~~~

GitHub: https://github.com/The-OpenROAD-Project/yosys

**Setup**

**Requirements**

-  C++ compiler with C++11 support (up-to-date CLANG or GCC is
   recommended)
-  GNU Flex, GNU Bison, and GNU Make.
-  TCL, readline and libffi.

On Ubuntu:

::

   $ sudo apt-get install build-essential clang bison flex \
           libreadline-dev gawk tcl-dev libffi-dev git \
           graphviz xdot pkg-config python3 libboost-system-dev \
           libboost-python-dev libboost-filesystem-dev zlib1g-dev

On Mac OS X Homebrew can be used to install dependencies (from within
cloned yosys repository):

::

   $ brew tap Homebrew/bundle && brew bundle

To configure the build system to use a specific compiler, use one of

::

   $ make config-clang
   $ make config-gcc

**Build**

To build Yosys simply type ‘make’ in this directory.

::

   $ make
   $ sudo make install

**Synthesis Script**

::

   yosys -import

   if {[info exist ::env(DC_NETLIST)]} {
   exec cp $::env(DC_NETLIST) $::env(RESULTS_DIR)/1_1_yosys.v
   exit
   }

   # Don't change these unless you know what you are doing
   set stat_ext    "_stat.rep"
   set gl_ext      "_gl.v"
   set abc_script  "+read_constr,$::env(SDC_FILE);strash;ifraig;retime,-D,{D},-M,6;strash;dch,-f;map,-p-M,1,{D},-f;topo;dnsize;buffer,-p;upsize;"


   # Setup verilog include directories
   set vIdirsArgs ""
   if {[info exist ::env(VERILOG_INCLUDE_DIRS)]} {
       foreach dir $::env(VERILOG_INCLUDE_DIRS) {
           lappend vIdirsArgs "-I$dir"
       }
       set vIdirsArgs [join $vIdirsArgs]
   }


   # read verilog files
   foreach file $::env(VERILOG_FILES) {
       read_verilog -sv {*}$vIdirsArgs $file
   }


   # Read blackbox stubs of standard cells. This allows for standard cell (or
   # structural netlist) support in the input verilog
   read_verilog $::env(BLACKBOX_V_FILE)

   # Apply toplevel parameters (if exist)
   if {[info exist ::env(VERILOG_TOP_PARAMS)]} {
       dict for {key value} $::env(VERILOG_TOP_PARAMS) {
           chparam -set $key $value $::env(DESIGN_NAME)
       }
   }

   # Read platform specific mapfile for OPENROAD_CLKGATE cells
   if {[info exist ::env(CLKGATE_MAP_FILE)]} {
       read_verilog $::env(CLKGATE_MAP_FILE)
   }

   # Use hierarchy to automatically generate blackboxes for known memory macro.
   # Pins are enumerated for proper mapping
   if {[info exist ::env(BLACKBOX_MAP_TCL)]} {
       source $::env(BLACKBOX_MAP_TCL)
   }

   # generic synthesis
   synth  -top $::env(DESIGN_NAME) -flatten

   # Optimize the design
   opt -purge

   # technology mapping of latches
   if {[info exist ::env(LATCH_MAP_FILE)]} {
       techmap -map $::env(LATCH_MAP_FILE)
   }

   # technology mapping of flip-flops
   dfflibmap -liberty $::env(OBJECTS_DIR)/merged.lib
   opt

   # Technology mapping for cells
   abc -D [expr $::env(CLOCK_PERIOD) * 1000] \
       -constr "$::env(SDC_FILE)" \
       -liberty $::env(OBJECTS_DIR)/merged.lib \
       -script $abc_script \
       -showtmp

   # technology mapping of constant hi- and/or lo-drivers
   hilomap -singleton \
           -hicell {*}$::env(TIEHI_CELL_AND_PORT) \
           -locell {*}$::env(TIELO_CELL_AND_PORT)

   # replace undef values with defined constants
   setundef -zero

   # Splitting nets resolves unwanted compound assign statements in netlist (assign {..} = {..})
   splitnets

   # insert buffer cells for pass through wires
   insbuf -buf {*}$::env(MIN_BUF_CELL_AND_PORTS)

   # remove unused cells and wires
   opt_clean -purge

   # reports
   tee -o $::env(REPORTS_DIR)/synth_check.txt check
   tee -o $::env(REPORTS_DIR)/synth_stat.txt stat -liberty $::env(OBJECTS_DIR)/merged.lib

   # write synthesized design
   write_verilog -noattr -noexpr -nohex -nodec $::env(RESULTS_DIR)/1_1_yosys.v

Initialize Floorplan
~~~~~~~~~~~~~~~~~~~~

::

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

If no -tracks file is used the routing layers from the LEF are used.

::

   core_area = design_area / (utilization / 100)
   core_width = sqrt(core_area / aspect_ratio)
   core_height = core_width * aspect_ratio
   core = ( core_space, core_space ) ( core_space + core_width, core_space + core_height )
   die = ( 0, 0 ) ( core_width + core_space * 2, core_height + core_space * 2 )

Place pins around core boundary.

::

   auto_place_pins pin_layer

Gate Resizer
~~~~~~~~~~~~

Gate resizer commands are described below. The resizer commands stop
when the design area is ``-max_utilization util`` percent of the core
area. ``util`` is between 0 and 100.

::

   set_wire_rc [-layer layer_name]
               [-resistance res ]
           [-capacitance cap]
           [-corner corner_name]

The ``set_wire_rc`` command sets the resistance and capacitance used to
estimate delay of routing wires. Use ``-layer`` or ``-resistance`` and
``-capacitance``. If ``-layer`` is used, the LEF technology resistance
and area/edge capacitance values for the layer are used. The units for
``-resistance`` and ``-capacitance`` are from the first liberty file
read, resistance_unit/distance_unit and liberty
capacitance_unit/distance_unit. RC parasitics are added based on placed
component pin locations. If there are no component locations no
parasitics are added. The resistance and capacitance are per distance
unit of a routing wire. Use the ``set_units`` command to check units or
``set_cmd_units`` to change units. They should represent “average”
routing layer resistance and capacitance. If the set_wire_rc command is
not called before resizing, the default_wireload model specified in the
first liberty file or with the SDC set_wire_load command is used to make
parasitics.

::

   buffer_ports [-inputs]
           [-outputs]
           -buffer_cell buffer_cell

The ``buffer_ports -inputs`` command adds a buffer between the input and
its loads. The ``buffer_ports -outputs`` adds a buffer between the port
driver and the output port. If The default behavior is ``-inputs`` and
``-outputs`` if neither is specified.

::

   resize [-libraries resize_libraries]
       [-dont_use cells]
       [-max_utilization util]

The ``resize`` command resizes gates to normalize slews.

The ``-libraries`` option specifies which libraries to use when
resizing. ``resize_libraries`` defaults to all of the liberty libraries
that have been read. Some designs have multiple libraries with different
transistor thresholds (Vt) and are used to trade off power and speed.
Chosing a low Vt library uses more power but results in a faster design
after the resizing step. Use the ``-dont_use`` option to specify a list
of patterns of cells to not use. For example, ``*/DLY*`` says do not use
cells with names that begin with ``DLY`` in all libraries.

::

   repair_max_cap -buffer_cell buffer_cell
               [-max_utilization util]
   repair_max_slew -buffer_cell buffer_cell
                   [-max_utilization util]

The ``repair_max_cap`` and ``repair_max_slew`` commands repair nets with
maximum capacitance or slew violations by inserting buffers in the net.

::

   repair_max_fanout -max_fanout fanout
                   -buffer_cell buffer_cell
                   [-max_utilization util]

The ``repair_max_fanout`` command repairs nets with a fanout greater
than ``fanout`` by inserting buffers between the driver and the loads.
Buffers are located at the center of each group of loads.

::

   repair_tie_fanout [-max_fanout fanout]
                   [-verbose]
                   lib_port

The ``repair_tie_fanout`` command repairs tie high/low nets with fanout
greater than ``fanout`` by cloning the tie high/low driver. ``lib_port``
is the tie high/low port, which can be a library/cell/port name or
object returned by ``get_lib_pins``. Clones are located at the center of
each group of loads.

::

   repair_hold_violations -buffer_cell buffer_cell
                       [-max_utilization util]

The ``repair_hold_violations`` command inserts buffers to repair hold
check violations.

::

   report_design_area

The ``report_design_area`` command reports the area of the design’s
components and the utilization.

::

   report_floating_nets [-verbose]

The ``report_floating_nets`` command reports nets with only one pin
connection. Use the ``-verbose`` flag to see the net names.

A typical resizer command file is shown below.

::

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

Note that OpenSTA commands can be used to report timing metrics before
or after resizing the design.

::

   set_wire_rc -layer metal2
   report_checks
   report_tns
   report_wns
   report_checks

   resize

   report_checks
   report_tns
   report_wns

Timing Analysis
~~~~~~~~~~~~~~~

Timing analysis commands are documented in src/OpenSTA/doc/OpenSTA.pdf.

After the database has been read from LEF/DEF, Verilog or an OpenDB
database, use the ``read_liberty`` command to read Liberty library files
used by the design.

The example script below timing analyzes a database.

::

   read_liberty liberty1.lib
   read_db reg1.db
   create_clock -name clk -period 10 {clk1 clk2 clk3}
   set_input_delay -clock clk 0 {in1 in2}
   set_output_delay -clock clk 0 out
   report_checks

MacroPlace
~~~~~~~~~~

TritonMacroPlace
https://github.com/The-OpenROAD-Project/TritonMacroPlace

::

   macro_placement -global_config <global_config_file>

-  **global_config** : Set global config file loction. [string]

Global Config Example
^^^^^^^^^^^^^^^^^^^^^

::

   set ::HALO_WIDTH_V 1
   set ::HALO_WIDTH_H 1
   set ::CHANNEL_WIDTH_V 0
   set ::CHANNEL_WIDTH_H 0

-  **HALO_WIDTH_V** : Set macro’s vertical halo. [float; unit: micron]
-  **HALO_WIDTH_H** : Set macro’s horizontal halo. [float; unit: micron]
-  **CHANNEL_WIDTH_V** : Set macro’s vertical channel width. [float;
   unit: micron]
-  **CHANNEL_WIDTH_H** : Set macro’s horizontal channel width. [float;
   unit: micron]

Tapcell
~~~~~~~

Tapcell and endcap insertion.

::

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

You can find script examples for supported technologies
``tapcell/etc/scripts``

Global Placement
~~~~~~~~~~~~~~~~

RePlAce global placement.
https://github.com/The-OpenROAD-Project/RePlAce

::

   global_placement -skip_initial_place
                    -incremental
                    -bin_grid_count <grid_count>
                    -density <density>
                    -init_density_penalty <init_density_penalty>
                    -init_wirelength_coef <init_wirelength_coef>
                    -min_phi_coef <min_phi_coef>
                    -max_phi_coef <max_phi_coef>
                    -overflow <overflow>
                    -initial_place_max_iter <max_iter>
                    -initial_place_max_fanout <max_fanout>
                    -verbose_level <level>

Flow Control
^^^^^^^^^^^^

-  **skip_initial_place** : Skip the initial placement (BiCGSTAB
   solving) before Nesterov placement. IP improves HPWL by ~5% on large
   designs. Equal to ‘-initial_place_max_iter 0’
-  **incremental** : Enable the incremental global placement. Users
   would need to tune other parameters (e.g. init_density_penalty) with
   pre-placed solutions.

Tuning Parameters
^^^^^^^^^^^^^^^^^

-  **bin_grid_count** : Set bin grid’s counts. Default: Defined by
   internal algorithm. [64,128,256,512,…, int]
-  **density** : Set target density. Default: 0.70 [0-1, float]
-  **init_density_penalty** : Set initial density penalty. Default: 8e-5
   [1e-6 - 1e6, float]
-  \__init_wire_length__coef_\_ : Set initial wirelength coefficient.
   Default: 0.25 [unlimited, float]
-  **min_phi_coef** : Set pcof_min(µ_k Lower Bound). Default: 0.95
   [0.95-1.05, float]
-  **max_phi_coef** : Set pcof_max(µ_k Upper Bound). Default: 1.05
   [1.00-1.20, float]
-  **overflow** : Set target overflow for termination condition.
   Default: 0.1 [0-1, float]
-  **initial_place_max_iter** : Set maximum iterations in initial place.
   Default: 20 [0-, int]
-  **initial_place_max_fanout** : Set net escape condition in initial
   place when ‘fanout >= initial_place_max_fanout’. Default: 200 [1-,
   int]

Other Options
^^^^^^^^^^^^^

-  **verbose_level** : Set verbose level for RePlAce. Default: 1 [0-10,
   int]

Detailed Placement
~~~~~~~~~~~~~~~~~~

Legalize a design that has been globally placed.

::

   legalize_placement [-constraints constraints_file]

Clock Tree Synthesis
~~~~~~~~~~~~~~~~~~~~

Create clock tree subnets.

::

   clock_tree_synthesis -lut_file <lut_file> \
                       -sol_list <sol_list_file> \
                       -wire_unit <wire_unit> \
                       -root_buf <root_buf> \
                       [-clk_nets <list_of_clk_nets>]

-  ``lut_file``, ``sol_list`` and ``wire_unit`` are parameters related
   to the technology characterization described
   `here <https://github.com/The-OpenROAD-Project/TritonCTS/blob/master/doc/Technology_characterization.md>`__.
-  ``root_buffer`` is the master cell of the buffer that serves as root
   for the clock tree.
-  ``clk_nets`` is a string containing the names of the clock roots. If
   this parameter is ommitted, TritonCTS looks for the clock roots
   automatically.

Global Routing
~~~~~~~~~~~~~~

FastRoute global route. Generate routing guides given a placed design.

::

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

Options description:

-  **capacity_adjustment**: Set global capacity adjustment (e.g.:
   -capacity_adjustment *0.3*)

-  **min_routing_layer**: Set minimum routing layer (e.g.:
   -min_routing_layer *2*)

-  **max_routing_layer**: Set maximum routing layer (e.g.:
   max_routing_layer *9*)

-  **pitches_in_tile**: Set the number of pitches inside a GCell

-  **layers_adjustments**: Set capacity adjustment to specific layers
   (e.g.: -layers_adjustments {{ } …})

-  **regions_adjustments**: Set capacity adjustment to specific regions
   (e.g.: -regions_adjustments { } …})

-  **nets_alphas_priorities**: Set alphas for specific nets when using
   clock net routing (e.g.: -nets_alphas_priorities {{ } …})

-  **verbose**: Set verbose of report. 0 for less verbose, 1 for medium
   verbose, 2 for full verbose (e.g.: -verbose 1)

-  **unidirectional_routing**: Activate unidirectional routing *(flag)*

-  **clock_net_routing**: Activate clock net routing *(flag)*

-  **NOTE 1:** if you use the flag *unidirectional_routing*, the minimum
   routing layer will be assigned as “2” automatically

-  **NOTE 2:** the first routing layer of the design have index equal to
   1

-  **NOTE 3:** if you use the flag *clock_net_routing*, only guides for
   clock nets will be generated

Detailed Routing
~~~~~~~~~~~~~~~~

**Run**

::

   detailed_route -param <param_file>

Options description:

-  **param_file**: This file contains the parameters used to
   control the detailed router)

