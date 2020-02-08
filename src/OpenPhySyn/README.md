# OpenPhySyn

OpenPhySyn is a plugin-based physical synthesis optimization kit developed as part of the [OpenROAD](https://theopenroadproject.org/) flow.

## Building

Build by creating a build directory (i.e. `build/`), run `cmake ..` in that directory, and then use `make` to build the desired target.

Example:

```bash
> mkdir build && cd build
> cmake .. -DCMAKE_INSTALL_PREFIX=${HOME}/apps/OpenPhySyn -DCMAKE_BUILD_TYPE=[Debug | Coverage | Release]
> make
> make install
> ./Psn
> make test # Makes and runs the tests.
> make coverage # Generate a coverage report.
> make doc # Generate html documentation.
```

## Building Custom Transforms

Physical Synthesis transforms libraries are loaded from the directory referred to by the variable `PSN_TRANSFORM_PATH`, defaulting to `./transforms`.

To build a new transform, refer to the transform [template](https://github.com/The-OpenROAD-Project/OpenPhySynHelloTransform).

## Getting Started

### Read design and run a transorm:

```bash
> ./Psn
> import_lef <lef file>
> import_def <def file>
> transform <transform name> <arguments...>
> export_def <def file>
```

### List available commands:
*The following commands are for the standalone version, for the OpenROAD version, please check the top-level [repository](https://github.com/The-OpenROAD-Project/OpenROAD).*

```bash
> ./Psn
> help
print_version                         Print version
version                               Print version
help                                  Print help
print_usage                           Print help
print_transforms                      List loaded transforms
import_lef <file path>                Load LEF file
import_def <file path>                Load DEF file
import_lib <file path>                Load a liberty file
import_liberty <file path>            Load a liberty file
export_def <output file>              Write DEF file
set_wire_rc <res> <cap>               Set resistance & capacitance per micron
set_max_area <area>                   Set maximum design area
optimize_design [<options>]           Perform timing optimization on the design
optimize_power [<options>]            Perform power optimization on the design
optimize_fanout <options>             Buffer high-fanout nets
transform <transform name> <args>     Run transform on the loaded design
has_transform <transform name>        Checks if a specific transform is loaded
design_area                           Returns total design cell area
link <design name>                    Link design top module
link_design <design name>             Link design top module
sta <OpenSTA commands>                Run OpenSTA commands
make_steiner_tree <net>               Construct steiner tree for the provided net
set_log <log level>                   Set log level [trace, debug, info, warn, error, critical, off]
set_log_level <log level>             Set log level [trace, debug, info, warn, error, critical, off]
set_log_pattern <pattern>             Set log printing pattern, refer to spdlog logger for pattern formats
```

### List loaded transforms:
```bash
> ./Psn
> print_transforms
```

### Print usage information for a transform:
```bash
> ./Psn
> transform <transform name> help
```

### Running OpenSTA commands:
```bash
> ./Psn
> import_lef <lef file>
> import_def <def file>
> sta create_clock [sta get_ports clk] -name core_clock -period 10
> sta report_checks
```

## Default Transforms

By default, the following transforms are built with OpenPhySyn:

- `hello_transform`: a demo transform that adds a random wire.
- `buffer_fanout`: adds buffers to high fan-out nets.
- `gate_clone`: performs load driven gate cloning.
- `pin_swap`: performs timing-driven/power-driven commutative pin-swapping optimization.

## Optimization Command(s)

The main provided command for timing optimization is `optimize_design`.

Currently, it performs pin swapping and load-driven gate-cloning to enhance the design timing.

Available options are:
- `[-no_gate_clone]`: Disable gate-cloning.
- `[-no_pin_swap]`: Disable pin-swap.
- `[-clone_max_cap_factor <factor>]`: Set gate-cloning capacitance load ratio, defaults to *1.5*.
- `[-clone_non_largest_cells]`: Allow cloning of cells that are not the largest of their cell-footprint, not recommended.

## Dependencies

OpenPhySyn depends on the following libraries:

- [CPP TaskFlow](https://github.com/cpp-taskflow/cpp-taskflow) [included, optional]
- [Flute](https://github.com/The-OpenROAD-Project/flute3) [included]
- [OpenSTA](https://github.com/The-OpenROAD-Project/OpenSTA) [included]
- [OpenDB](https://github.com/The-OpenROAD-Project/OpenDB) [included]
- [cxxopts](https://github.com/jarro2783/cxxopts) [included]
- [SWIG](http://www.swig.org/Doc1.3/Tcl.html)
- [Doxygen](http://www.doxygen.nl) [included, optional]
- [Doctests](https://github.com/onqtam/doctest) [included, optional]

## Issues

Please open a GitHub [issue](https://github.com/The-OpenROAD-Project/OpenPhySyn/issues/new) if you find any bugs.

## To-Do

- [x] Integrate Steiner Tree
- [x] Integrate Liberty Parser
- [x] Integrate OpenSTA
- [x] Support reading scripts from a file
- [x] Add unit tests
- [x] Add Gate Cloning Transform
- [x] Fix issues with OpenSTA commands integration
- [x] Support command-line options for Tcl functions instead of just positional arguments
- [x] Support passing parsed options to the transform body
- [ ] Fix inconsistencies in the DB handlers between `const char *` and `std::string` 
- [ ] Expose lower-level APIs through Tcl/Python interface
- [ ] Add profiling tool (i.e., Valgrind).
- [ ] Add API documentation
- [ ] Add Coding Guideline and Contribution Guide
