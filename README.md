# OpenStaDB, OpenSTA on OpenDB

OpenStaDB the OpenSTA static timing analyzer that uses an OpenDB
database for netlist connectivity.

#### Installation

OpenStaDB depends on OpenSTA, and OpenDB. These source directories are
git submodules and located in `/module`.

```
git clone --recursive https://github.com/The-OpenROAD-Project/OpenStaDB.git
cd OpenStaDB
mkdir build
cd build
cmake .. -DBUILD_PYTHON=OFF -DBUILD_TCL=OFF
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

#### Running OpenStaDB

```
opensta_db
  -help              show help and exit
  -version           show version and exit
  -no_init           do not read .sta init file
  -no_splash         do not show the license splash at startup
  -exit              exit after reading cmd_file
  cmd_file           source cmd_file
```

opensta_db sources the TCL command file `~/.sta` unless the command
line option `-no_init` is specified.

opensta_db then sources the command file cmd_file. Unless the `-exit`
command line flag is specified it enters and interactive TCL command
interpreter.

OpenStaDB is run using TCL scripts.  In addition to the OpenSTA
commands documented in OpenSTA/doc/Sta.pdf, available commands are
shown below.

```
read_lef [-tech] [-library] filename
read_def filename
write_def filename
read_verilog filename
write_verilog filename
read_db filename
write_db filename
init_sta_db
```

OpenStaDB can be used to make and OpenDB database from LEF/DEF, or
Verilog (flat or hierarchical). Once the database is made it can be
saved as a file with the `write_db` command. OpenStaDB can then read
the database with the `read_db` command without reading LEF/DEF or
Verilog.

The `read_lef` and `read_def` commands can be used to build an OpenDB
database as shown below. In this example the LEF contains both
technology and MACRO definitions so the `-tech` and `-library` flags
are used with the `read_lef` command.

```
read_lef -tech -library liberty1.lef
read_def reg1.def
# Write the db for future runs.
write_db reg1.def
```

The `read_verilog` command is used to build an OpenDB database as
shown below. Multiple verilog files for a hierarchical design can be
read.  The `link_design` command is used to flatten the design
and make a database.

```
read_lef -tech -library liberty1.lef
read_verilog reg1.v
link_design top
# Write the db for future runs.
write_db reg1.db
```

After the database has been read from LEF/DEF, Verilog or an OpenDB
database, use the `read_liberty` command to read Liberty library files
used by the design.

The `init_sta_db` command is used to initialize OpenSTA after the database
is built and liberty files have been read.

The example script below timing analyzes a database.

```
read_db reg1.db
read_liberty liberty1.lib
init_sta_db
create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}
set_output_delay -clock clk 0 out
report_checks
```

## Authors

* James Cherry
