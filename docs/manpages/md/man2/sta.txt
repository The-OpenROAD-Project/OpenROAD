# Parallax Static Timing Analyzer

OpenSTA is a gate level static timing verifier. As a stand-alone
executable it can be used to verify the timing of a design using
standard file formats.

* Verilog netlist
* Liberty library
* SDC timing constraints
* SDF delay annotation
* SPEF parasitics

OpenSTA uses a TCL command interpreter to read the design, specify
timing constraints and print timing reports.

##### Clocks
* Generated
* Latency
* Source latency (insertion delay)
* Uncertainty
* Propagated/Ideal
* Gated clock checks
* Multiple frequency clocks

##### Exception paths
* False path
* Multicycle path
* Min/Max path delay
* Exception points
*  -from clock/pin/instance -through pin/net -to clock/pin/instance
*  Edge specific exception points
*   -rise_from/-fall_from, -rise_through/-fall_through, -rise_to/-fall_to

##### Delay calculation
* Integrated Dartu/Menezes/Pileggi RC effective capacitance algorithm
* External delay calculator API

##### Analysis
* Report timing checks -from, -through, -to, multiple paths to endpoint
* Report delay calculation
* Check timing setup

##### Timing Engine
OpenSTA is architected to be easily bolted on to other tools as a
timing engine.  By using a network adapter, OpenSTA can access the host
netlist data structures without duplicating them.

* Query based incremental update of delays, arrival and required times
* Simulator to propagate constants from constraints and netlist tie high/low

See doc/OpenSTA.pdf for command documentation.
See doc/StaApi.txt for timing engine API documentation.
See doc/ChangeLog.txt for changes to commands.

OpenSTA is dual licensed. It is released under GPL v3 as OpenSTA and
is also licensed for commerical applications by Parallax Software without
the GPL's requirements.

OpenSTA is open source, meaning the sources are published and can be
compiled locally.  Derivative works are supported as long as they
adhere to the GPL license requirements.  However, OpenSTA is not
supported by a public community of developers as many other open
source projects are. The copyright and develpment are exclusive to
Parallax Software. OpenSTA does not solicit or accept external code
contributions.

Removing copyright and license notices from OpenSTA sources (or any
other open source project for that matter) is illegal. This should be
obvious, but the author of OpenSTA has discovered two different cases
where the copyright and license were removed from source files that
were copied.

The official git repository is located at
https://github.com/parallaxsw/OpenSTA.git. Any forks from this code
base have not passed extensive regression testing which is not
publicly available.

## Build

OpenSTA is built with CMake.

### Prerequisites

The build dependency versions are show below.  Other versions may
work, but these are the versions used for development.

```
         from   Ubuntu   Xcode
                22.04.2  11.3
cmake    3.10.2 3.24.2   3.16.2
clang    9.1.0           14.0.3
gcc      3.3.2   11.3.0  
tcl      8.4     8.6     8.6.6
swig     1.3.28  4.1.0   4.0.1
bison    1.35    3.0.2   3.8.2
flex     2.5.4   2.6.4   2.6.4
```

Note that flex versions before 2.6.4 contain 'register' declarations that
are illegal in c++17.

These packages are **optional**:

```
tclreadline                   2.3.8
libz        1.1.4   1.2.5     1.2.8
cudd                2.4.1     3.0.0
```

The [TCL readline library](https://tclreadline.sourceforge.net/tclreadline.html)
links the GNU readline library to the TCL interpreter for command line editing 
On OSX, Homebrew does not support tclreadline, but the macports system does
(see https://www.macports.org). To enable TCL readline support use the following
Cmake option:

```
cmake .. -DUSE_TCL_READLINE=ON
```

The Zlib library is an optional.  If CMake finds libz, OpenSTA can
read Verilog, SDF, SPF, and SPEF files compressed with gzip.

CUDD is a binary decision diageram (BDD) package that is used to
improve conditional timing arc handling. OpenSTA does not require it
to be installed. It is available
[here](https://www.davidkebo.com/source/cudd_versions/cudd-3.0.0.tar.gz)
or [here](https://sourceforge.net/projects/cudd-mirror/).

Note that the file hierarchy of the CUDD installation changed with version 3.0.
Some changes to CMakeLists.txt are required to support older versions.

Use the USE_CUDD option to look for the cudd library.
Use the CUDD_DIR option to set the install directory if it is not in
one of the normal install directories.

When building CUDD you may use the `--prefix ` option to `configure` to
install in a location other than the default (`/usr/local/lib`).
```
cd $HOME/cudd-3.0.0
mkdir $HOME/cudd
./configure --prefix $HOME/cudd
make
make install

cd <opensta>/build
cmake .. -DUSE_CUDD=ON -DCUDD_DIR=$HOME/cudd
```

### Installing with CMake

Use the following commands to checkout the git repository and build the
OpenSTA library and excutable.

```
git clone https://github.com/The-OpenROAD-Project/OpenSTA.git
cd OpenSTA
mkdir build
cd build
cmake ..
make
```
The default build type is release to compile optimized code.
The resulting executable is in `app/sta`.
The library without a `main()` procedure is `app/libSTA.a`.

Optional CMake variables passed as -D<var>=<value> arguments to CMake are show below.

```
CMAKE_BUILD_TYPE DEBUG|RELEASE
CMAKE_CXX_FLAGS - additional compiler flags
TCL_LIBRARY - path to tcl library
TCL_HEADER - path to tcl.h
CUDD - path to cudd installation
ZLIB_ROOT - path to zlib
CMAKE_INSTALL_PREFIX
```

If `TCL_LIBRARY` is specified the CMake script will attempt to locate
the header from the library path.

The default install directory is `/usr/local`.
To install in a different directory with CMake use:

```
cmake .. -DCMAKE_INSTALL_PREFIX=<prefix_path>
```

If you make changes to `CMakeLists.txt` you may need to clean out
existing CMake cached variable values by deleting all of the
files in the build directory.

## Bug Reports

Use the Issues tab on the github repository to report bugs.

Each issue/bug should be a separate issue. The subject of the issue
should be a short description of the problem. Attach a test case to
reproduce the issue as described below. Issues without test cases are
unlikely to get a response.

The files in the test case should be collected into a directory named
YYYYMMDD where YYYY is the year, MM is the month, and DD is the
day (this format allows "ls" to report them in chronological order).
The contents of the directory should be collected into a compressed
tarfile named YYYYMMDD.tgz.

The test case should have a tcl command file recreates the issue named
run.tcl. If there are more than one command file using the same data
files, there should be separate command files, run1.tcl, run2.tcl
etc. The bug report can refer to these command files by name.

Command files should not have absolute filenames like
"/home/cho/OpenSTA_Request/write_path_spice/dump_spice" in them.
These obviously are not portable. Use filenames relative to the test
case directory.

## Authors

* James Cherry

* William Scott authored the arnoldi delay calculator at Blaze, Inc which was subsequently licensed to Nefelus, Inc that has graciously contributed it to OpenSTA.

## License

OpenSTA, Static Timing Analyzer
Copyright (c) 2023, Parallax Software, Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
