# Developer Guide

## Tool Philosophy

OpenROAD is a tool to build a chip from a synthesized netlist to a
physical design for manufacturing.

The unifying principle behind the design of OpenROAD is for all of the
tools to reside in one tool, with one process, and one database. All
tools in the flow should use Tcl commands exclusively to control them
instead of external "configuration files". File based communication
between tools and forking processes is strongly discouraged. This
architecture streamlines the construction of a flexible tool flow and
minimizes the overhead of invoking each tool in the flow.

## Tool File Organization

Every tool follows the following file structure, grouping sources, tests
and headers together.

```
    src/CMakelists.txt - add_subdirectory's src/CMakelists.txt
    src/tool/src/ - sources and private headers
    src/tool/src/CMakelists.txt
    src/tool/include/tool/ - exported headers
    src/tool/test/
    src/tool/test/regression
```

OpenROAD repository:

```
    CMakeLists.txt - top level CMake file
    src/Main.cc
    src/OpenRoad.cc - OpenROAD class functions
    src/OpenRoad.i - top level swig, %includes tool swig files
    src/OpenRoad.tcl - basic read/write lef/def/db commands
    include/ord/OpenRoad.hh - OpenROAD top level class, has instances of tools
```

Some tools such as OpenSTA are submodules, which are simply
subdirectories in `src/` that are pointers to the git submodule. They are
intentionally not segregated into a separate /module.

The use of submodules for new code integrated into OpenROAD is strongly
discouraged. Submodules make changes to the underlying infrastructure
(i.e., OpenSTA) difficult to propagate across the dependent submodule
repositories. Submodules: just say no.

Where external/third party code that a tool depends on should be placed
depends on the nature of the dependency.

-   Libraries - code packaged as a linkable library. Examples are `tcl`,
    `boost`, `zlib`, `eigen`, `lemon`, `spdlog`.

These should be installed in the build environment and linked by
OpenROAD. Document these dependencies in the top level `README.md` file.
The `Dockerfile` should be updated to illustrate where to find the library
and how to install it. Adding libraries to the build environment requires
coordination with the sysadmins for the continuous integration hosts to
make sure the environments include the dependency. Advanced notification
should also be given to the development team so their private build
environments can be updated.

Each tool CMake file builds a library that is linked by the OpenROAD
application. The tools should not define a `main()` function. If the
tool is tcl only and has no C++ code it does not need to have a CMake
file. Tool CMake files should **not** include the following:

- `cmake_minimum_required`
- `GCC_COVERAGE_COMPILE_FLAGS`
- `GCC_COVERAGE_LINK_FLAGS`
- `CMAKE_CXX_FLAGS`
- `CMAKE_EXE_LINKER_FLAGS`

None of the tools have commands to read or write LEF, DEF, Verilog or
database files. These functions are all provided by the OpenROAD
framework for consistency.

Tools should package all state in a single class. An instance of each
tool class resides in the top level OpenROAD object. This allows
multiple tools to exist at the same time. If any tool keeps state in
global variables (even static) only one tool can exist at a time. Many
of the tools being integrated were not built with this goal in mind and
will only work on one design at a time. Eventually all of the tools
should be upgraded to remove this deficiency as they are re-written to
work in the OpenROAD framework.

Each tool should use a unique namespace for all of its code. The same
namespace should be used for Tcl functions, including those defined by a
swig interface file. Internal Tcl commands stay inside the namespace,
and user visible Tcl commands should be defined in the global namespace.
User commands should be simple Tcl commands such as 'global_placement'
that do not create tool instances that must be based to the commands.
Defining Tcl commands for a tool class is fine for internals, but not
for user visible commands. Commands have an implicit argument of the
current OpenROAD class object. Functions to get individual tools from
the OpenROAD object can be defined.

## Initialization (C++ tools only)

The OpenROAD class has pointers to each tools with functions to get each
tool. Each tool has (at a minimum) a function to make an instance of the
tool class, and an initialization function that is called after all of
the tools have been made, and a function to delete the tool. This small
header does **not** include the class definition for the tool so that
the OpenROAD framework does not have to know anything about the tool
internals or include a gigantic header file.

`MakeTool.hh` defines the following:

``` cpp
    Tool *makeTool();
    void initTool(OpenRoad *openroad);
    void deleteTool(Tool *tool);
```

The `OpenRoad::init()` function calls all of the `makeTool` functions and
then all of the `initTool()` functions. The `init` functions are called from
the bottom of the tool dependences. Each `init` function grabs the state
it needs out of the `OpenRoad` instance.

## Commands

Tools should provide Tcl commands to control them. Tcl object based tool
interfaces are not user friendly. Define Tcl procedures that take
keyword arguments that reference the `OpenRoad` object to get tool state.
OpenSTA has Tcl utilities to parse keyword arguments
(`sta::parse_keyword_args`). See `OpenSTA/tcl/*.tcl` for
examples. Use swig to define internal functions to C++ functionality.

Tcl files can be included by encoding them in CMake into a string that
is evaluated at run time (See `Resizer::init()`).

## Errors

Tools should report errors to the user using the `ord::error` function
defined in `include/openroad/Error.hh`. `ord::error` throws
`ord::Exception`. The variables `ord::exit_on_error` and
`ord::file_continue_on_error` control how the error is handled. If
`ord::exit_on_error` is `true` OpenROAD reports the error and exits. If
the error is encountered while reading a file with the `source` or
`read_sdc` commands and `ord::file_continue_on_error` is `false` no
other commands are read from the file. The default values of both
variables is `false`.

## Test

Each "tool" has a /test directory containing a script named
`regression` to run "unit" tests. With no arguments it should run
default unit tests.

No database files should be in tests. Read LEF/DEF/Verilog to make a
database.

The regression script should not depend on the current working
directory. It should be able to be run from any directory. Use filenames
relative to the script name rather the current working directory.

Regression scripts should print a concise summary of test failures. The
regression script should return an exit code of zero if there are no
errors and 1 if there are errors. The script should **not** print
thousands of lines of internal tool info.

Regression scripts should pass the `-no_init` option to openroad so that
a user's `init` file is not sourced before the tests runs.

Regression scripts should add output files or directories to
`.gitignore` so that running does note leave the source repository
"dirty".

The Nangate45 open source library data used by many tests is in
`test/Nangate45`. Use the following command to add a link in the tool
command

``` shell
    cd tool/test
    ln -s ../../../test/Nangate45
```

After the link is installed, the test script can read the liberty file
with the command shown below.

``` tcl
    read_liberty Nangate45/Nangate45_typ.lib
```

## Building

Instructions for building are available [here](../user/GettingStarted.md).

## Tool Work Flow

To work on one of the tools inside OpenROAD when it is a submodule
requires updating the OpenROAD repo to integrate your changes.
Submodules point to a specific version (hash) of the submodule repo and
do not automatically track changes to the submodule repo.

Work on OpenROAD should be done in the `openroad` branch. Stable commits
on the `openroad` branch are periodically pushed to the `master` branch
for public consumption.

To make changes to a submodule, first check out a branch of the
submodule (git clone --recursive does not check out a branch, just a
specific commit).

``` shell
    cd src/<tool>
    git checkout <branch>
```

`<branch>` is the branch used for development of the tool when it is
inside OpenROAD. The convention is for to be named 'openroad'.

After making changes inside the tool source tree, stage and commit them
to the tool repo and push them to the remote repo.

``` shell
    git add [...]
    git commit -m "massive improvement"
    git push
```

If instead you have done development in a different branch or source
tree, merge those changes into the branch used for OpenROAD.

Once the changes are in the OpenROAD submodule source tree it will show
them as a diff in the hash for the directory.

``` shell
    cd openroad
    git stage <tool_submodule_dir>
    git commit -m "merge tool massive improvement"
    git push
```

## Example of Adding a Tool to OpenROAD

The patch file "add_tool.patch" illustrates how to add a tool to
OpenROAD. Use

``` shell
    patch -p < doc/add_tool.patch`
    cd src/tool/test
    ln -s ../../../test/regression.tcl regression.tcl
```

To add the sample tool. This adds a directory `OpenRoad/src/tool` that
illustrates a tool named "Tool" that uses the file structure described
and defines a command to run the tool with keyword and flag arguments as
illustrated below:

``` tcl
    % toolize foo
    Helping 23/6
    Gotta pos_arg1 foo
    Gotta param1 0.000000
    Gotta flag1 false

    % toolize -flag1 -key1 2.0 bar
    Helping 23/6
    Gotta pos_arg1 bar
    Gotta param1 2.000000
    Gotta flag1 true

    % help toolize
    toolize [-key1 key1] [-flag1] pos_arg1
```

## Documentation

Tool commands should be documented in the top level OpenROAD `README.md`
file. Detailed documentation should be the `tool/README.md` file.

## Tool Flow

- Verilog to DB (dbSTA)
- Init Floorplan (OpenROAD)
- I/O placement (ioPlacer)
- PDN generation (pdngen)
- Tapcell and Welltie insertion (tapcell)
- I/O placement (ioPlacer)
- Macro placement (TritonMacroPlace)
- Global placement (RePlAce)
- Gate Resizing and buffering (Resizer)
- Detailed placement (OpenDP)
- Clock Tree Synthesis (TritonCTS)
- Repair Hold Violations (Resizer)
- Global route (FastRoute)
- Detailed route (TritonRoute)
- Final timing/power report (OpenSTA)

## Tool Checklist

Tools should make every attempt to minimize external dependencies.
Linking libraries other than those currently in use complicates the
builds and sacrifices the portability of OpenROAD. OpenROAD should be
portable to many different compiler/operating system versions and
dependencies make this vastly more complicated.

1. OpenROAD submodules reference tool `openroad` branch head. No git `develop`, `openroad_app`, or `openroad_build` branches.
1. Submodules used by more than one tool belong in `src/`, not duplicated in each tool repo.
1. `CMakeLists.txt` does not use add_compile_options include_directories link_directories link_libraries Use target\_ versions instead. See <https://gist.github.com/mbinna/c61dbb39bca0e4fb7d1f73b0d66a4fd1>
1. `CMakeLists.txt` does not use glob. Use explicit lists of source files and headers instead.
1. `CMakeLists.txt` does not define `CFLAGS` `CMAKE_CXX_FLAGS` `CMAKE_CXX_FLAGS_DEBUG` `CMAKE_CXX_FLAGS_RELEASE` Let the top level and defaults control these.
1. No `main.cpp` or main procedure.
1. No compiler warnings for GCC or Clang with optimization enabled.
1. Does not call `flute::readLUT` (called once by `openroad`).
1. Tcl command(s) documented in top level `README.md` in flow order.
1. Command line tool documentation in tool README.
1. Conforms to Tcl command naming standards (no camel case).
1. Does not read configuration files. Use command arguments or support commands.
1. `.clang-format` at tool root directory to aid foreign programmers.
1. No `jenkins/`, `Jenkinsfile`, `Dockerfile` in tool directory.
1. `regression` script named `test/regression` with no arguments that runs tests. Not `tests/regression-tcl.sh`, not `test/run_tests.py` etc.
1. `regression` script should run independent of current directory. For example, `../test/regression` should work.
1. `regression` should only print test results or summary, not belch 1000s of lines of output.
1. Test scripts use OpenROAD tcl commands (not `itcl`, not internal accessors).
1. `regression` script should only write files in a directory that is in the tool's `.gitignore` so the hierarchy does not have modified files in it as a result or running the regressions.
1. Regressions report no memory errors with `valgrind` (stretch goal).
1. Regressions report no memory leaks with `valgrind` (difficult).

James Cherry, Dec 2019
