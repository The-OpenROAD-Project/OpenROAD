# Developer Guide

## Tool Philosophy

OpenROAD is a tool to build a chip from synthesizable RTL (Verilog) to
completed physical layout (manufacturable, tapeout-clean GDSII).

The unifying principle behind the design of OpenROAD is for all of the
tools to reside in one tool, with one process, and one database. All
tools in the flow should use Tcl commands exclusively to control them
instead of external "configuration files". File-based communication
between tools and forking processes is strongly discouraged. This
architecture streamlines the construction of a flexible tool flow and
minimizes the overhead of invoking each tool in the flow.

## Tool File Organization

Every tool follows the following file structure, grouping sources, tests
and headers together.

- `src/`
   This folder contains the source files for individual tools.

| `src`           | Purpose |
|-----------------|--------------|
| `CMakeLists.txt`  | `add_subdirectory` for each tool|
| `tool/src`       | sources and private headers |
| `tool/src/CMakeLists.txt` | tool specific CMake file |
| `tool/include/tool` | exported headers |
| `tool/test`       | tool tests |
| `tool/regression` | tool unit tests|

- OpenROAD repository:
  This folder contains the top-level files for overall compilation. OpenROAD uses [swig](https://swig.org/) that acts as a wrapper for C/C++ programs to be callable in higher-level languages, such as Python and Tcl.

| `OpenROAD`      | Purpose |
|-----------------|--------------|
| `CMakeLists.txt` | top-level CMake file |
| `src/Main.cc`   | main file  |
| `src/OpenROAD.cc` | OpenROAD class functions |
| `src/OpenROAD.i`  | top-level swig, includes, tool swig files |
| `src/OpenROAD.tcl` | basic read/write lef/def/db commands |
| `include/ord/OpenROAD.hh` | OpenROAD top-level class, has instances of tools |

Some tools such as OpenSTA are submodules, which are simply
subdirectories in `src/` that are pointers to the git submodule. They are
intentionally not segregated into a separate module.

The use of submodules for new code integrated into OpenROAD is strongly
discouraged. Submodules make changes to the underlying infrastructure
(e.g., OpenSTA) difficult to propagate across the dependent submodule
repositories.

Where external/third-party code that a tool depends on should be placed
depends on the nature of the dependency.

-   Libraries - code packaged as a linkable library. Examples are `tcl`,
    `boost`, `zlib`, `eigen`, `lemon`, `spdlog`.

These should be installed in the build environment and linked by
OpenROAD. Document these dependencies in the top-level `README.md` file.
The `Dockerfile` should be updated to illustrate where to find the library
and how to install it. Adding libraries to the build environment requires
coordination with system administrators, so that continuous integration hosts ensure
that environments include the dependency. Advance notification
should also be given to the development team so that their private build
environments can be updated.

Each tool CMake file builds a library that is linked by the OpenROAD
application. The tools should not define a `main()` function. If the
tool is Tcl only and has no C++ code, it does not need to have a CMake
file. Tool CMake files should **not** include the following:

- `cmake_minimum_required`
- `GCC_COVERAGE_COMPILE_FLAGS`
- `GCC_COVERAGE_LINK_FLAGS`
- `CMAKE_CXX_FLAGS`
- `CMAKE_EXE_LINKER_FLAGS`

None of the tools have commands to read or write LEF, DEF, Verilog or
database files. For consistency, these functions are all provided by the OpenROAD
framework.

Tools should package all of their state in a single class. An instance of each
tool class resides in the top-level OpenROAD object. This allows
multiple tools to exist at the same time. If any tool keeps state in
global variables (even static), then only one tool can exist at a time. Many
of the tools being integrated were not built with this goal in mind and
will only work on one design at a time.

Each tool should use a unique namespace for all of its code. The same
namespace should be used for Tcl functions, including those defined by a
swig interface file. Internal Tcl commands stay inside the namespace,
and user visible Tcl commands should be defined in the global namespace.
User commands should be simple Tcl commands such as `global_placement`
that do not create tool instances that must be based to the commands.
Defining Tcl commands for a tool class is fine for internal commands, but not
for user visible commands. Commands have an implicit argument of the
current OpenROAD class object. Functions to get individual tools from
the OpenROAD object can be defined.

## Initialization (C++ tools only)

The OpenROAD class has pointers to each tool, with functions to get each
tool. Each tool has (at a minimum) a function to make an instance of the
tool class, an initialization function that is called after all of
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
the bottom of the tool dependencies. Each `init` function grabs the state
it needs out of the `OpenRoad` instance.

## Commands

Tools should provide Tcl commands to control them. Tcl object based tool
interfaces are not user-friendly. Define Tcl procedures that take
keyword arguments that reference the `OpenRoad` object to get tool state.
OpenSTA has Tcl utilities to parse keyword arguments
(`sta::parse_keyword_args`). See `OpenSTA/tcl/*.tcl` for
examples. Use swig to define internal functions to C++ functionality.

Tcl files can be included by encoding them in CMake into a string that
is evaluated at run time (See [`Resizer::init()`](../main/src/rsz/src/Resizer.cc)).

:::{Note}
Please refer to the top-level Tcl formatting [guide](TclFormat.md).
Our top-level Tcl files, in particular, have to be formatted in this specific
manner because of the automatic parsing used to convert the READMEs into
manpages.
:::

## Errors

Tools should report errors to the user using the `ord::error` function
defined in `include/openroad/Error.hh`. `ord::error` throws
`ord::Exception`. The variables `ord::exit_on_error` and
`ord::file_continue_on_error` control how the error is handled. If
`ord::exit_on_error` is `true` then OpenROAD reports the error and exits. If
the error is encountered while reading a file with the `source` or
`read_sdc` commands and `ord::file_continue_on_error` is `false` then no
other commands are read from the file. The default value is `false` for both
variables.

## Test

Each "tool" has a `/test` directory containing a script named
`regression` to run "unit" tests. With no arguments it should run
default unit tests.

No database files should be in tests. Read LEF/DEF/Verilog to make a
database.

The regression script should not depend on the current working
directory. It should be able to be run from any directory. Use filenames
relative to the script name rather the current working directory.

Regression scripts should print a concise summary of test failures. The
regression script should return an exit code of 0 if there are no
errors and 1 if there are errors. The script should **not** print
thousands of lines of internal tool information.

Regression scripts should pass the `-no_init` option to `openroad` so that
a user's `init` file is not sourced before the tests runs.

Regression scripts should add output files or directories to
`.gitignore` so that running does not leave the source repository
"dirty".

The Nangate45 open-source library data used by many tests is in
`test/Nangate45`. Use the following command to add a link in the tool command:

``` shell
cd src/<tool>/test
ln -s ../../../test/Nangate45
```

After the link is installed, the test script can read the Liberty file
with the command shown below.

``` tcl
read_liberty Nangate45/Nangate45_typ.lib
```

## Building

Instructions for building are available [here](../user/Build.md).

## Example of Adding a Tool to OpenROAD

The patch file "AddTool.patch" illustrates how to add a tool to
OpenROAD. Use the following commands to add a sample tool:

``` shell
# first, update existing config files
patch -p1 < docs/misc/AddTool.patch

# next, create the additional source files of the tool using this command
patch -p1 < docs/misc/AddToolFiles.patch

# finally, create the regression tests as follows
cd src/tool/test
ln -s ../../../test/regression.tcl regression.tcl
```

This adds a directory `OpenRoad/src/tool` that
illustrates a tool named "Tool" that uses the file structure described above
and defines a command to run the tool with keyword and flag arguments as
illustrated below:

```tcl
> toolize foo
Helping 23/6
Gotta positional_argument1 foo
Gotta param1 0.000000
Gotta flag1 false

> toolize -flag1 -key1 2.0 bar
Helping 23/6
Gotta positional_argument2 bar
Gotta param1 2.000000
Gotta flag1 true

> help toolize
toolize [-key1 key1] [-flag1] positional_argument1
```

## Documentation

Tool commands should be documented in the top-level OpenROAD `README.md`
file. Detailed documentation should be the `tool/README.md` file.

:::{Note}
Please refer to the README formatting [guide](ReadmeFormat.md).
Our top-level READMEs, in particular, have to be formatted in this specific
manner because of the automatic parsing used to convert the READMEs into
manpages.
:::

## Tool Flow Namespace

Tool namespaces are usually three-lettered lowercase letters.

- Verilog to DB (dbSTA)
- OpenDB: Open Database ([odb](../main/src/odb/README.md))
- TritonPart: constraints-driven paritioner ([par](../main/src/par/README.md))
- Floorplan Initialization ([ifp](../main/src/ifp/README.md))
- ICeWall chip-level connections ([pad](../main/src/pad/README.md))
- I/O Placement ([ppl](../main/src/ppl/README.md))
- PDN Generation ([pdn](../main/src/pdn/README.md))
- Tapcell and Welltie Insertion ([tap](../main/src/tap/README.md))
- Triton Macro Placer ([mpl](../main/src/mpl/README.md))
- Hierarchical Automatic Macro Placer ([mpl2](../main/src/mpl2/README.md))
- RePlAce Global Placer ([gpl](../main/src/gpl/README.md))
- Gate resizing and buffering ([rsz](../main/src/rsz/README.md))
- Detailed placement ([dpl](../main/src/dpl/README.md))
- Clock tree synthesis ([cts](../main/src/cts/README.md))
- FastRoute Global routing ([grt](../main/src/grt/README.md))
- Antenna check and diode insertion ([ant](../main/src/ant/README.md))
- TritonRoute Detailed routing ([drt](../main/src/drt/README.md))
- Metal fill insertion ([fin](../main/src/fin/README.md))
- Design for Test ([dft](../main/src/dft/README.md))
- OpenRCX Parasitic Extraction ([rcx](../main/src/rcx/README.md))
- OpenSTA timing/power analyzer ([sta](https://github.com/The-OpenROAD-Project/OpenSTA/blob/master/README.md)
- Graphical User Interface ([gui](../main/src/gui/README.md))
- Static IR analyzer ([psm](../main/src/psm/README.md))

## Tool Checklist

Tools should make every attempt to minimize external dependencies.
Linking libraries other than those currently in use complicates the
builds and sacrifices the portability of OpenROAD. OpenROAD should be
portable to many different compiler/operating system versions and
dependencies make this vastly more complicated.

1. OpenROAD submodules reference tool `openroad` branch head. No git `develop`, `openroad_app`, or `openroad_build` branches.
1. Submodules used by more than one tool belong in `src/`, not duplicated in each tool repo.
1. `CMakeLists.txt` does not use add_compile_options include_directories link_directories link_libraries. Use target\_ versions instead. See tips [here](https://gist.github.com/mbinna/c61dbb39bca0e4fb7d1f73b0d66a4fd1).
1. `CMakeLists.txt` does not use glob. Use explicit lists of source files and headers instead.
1. `CMakeLists.txt` does not define `CFLAGS` `CMAKE_CXX_FLAGS` `CMAKE_CXX_FLAGS_DEBUG` `CMAKE_CXX_FLAGS_RELEASE`. Let the top level and defaults control these.
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
1. Ensure the top-level README and Tcl format are compliant.

## Code Linting and Formatting

OpenROAD uses both `clang-tidy` and `clang-format` to perform automatic linting and formatting whenever a pull request is submitted. To run these locally, please first setup Clang Tooling using this [guide](https://clang.llvm.org/docs/HowToSetupToolingForLLVM.html). Thereafter, you may run these commands:

```shell
cmake . -B build  # generate build files
# typically only run these commands on files you changed.
clang-tidy -p ./build source_file.cpp
clang-format -i -style=file:.clang-format source_file.cpp
```

To run `clang-tidy` on all files, you can use the following script that runs
`clang-tidy` in parallel and also caches the results, so subsequent runs
only have to operate on changed files.

```shell
cmake . -B build  # generate build files
ln -sf build/compile_commands.json .  # make compilation db visible
/bin/sh etc/run-clang-tidy-cached.cc
```

## Doxygen

OpenROAD uses Doxygen style comments to generate documentation.
See the generated documentation <a href="../doxygen_output/html/index.html">here</a>.
Our preferred syntax for Doxygen comments can be found in this
[file](../../src/odb/include/odb/odb.h). Also, do refer to the official Doxygen
documentation for more information on what you can include in your Doxygen
comments [here](https://www.doxygen.nl/manual/docblocks.html).

Below shows an example snippet taken from `./src/odb/include/odb/db.h`:

```cpp
///
/// dbProperty - Int property.
///
class dbIntProperty : public dbProperty
{
 public:
  /// Get the value of this property.
  int getValue();

  /// Set the value of this property.
  void setValue(int value);

  /// Create a int property. Returns nullptr if a property with the same name
  /// already exists.
  static dbIntProperty* create(dbObject* object, const char* name, int value);

  /// Find the named property of type int. Returns nullptr if the property does
  /// not exist.
  static dbIntProperty* find(dbObject* object, const char* name);
};
```

## Guidelines

1. Internally, the code should use `int` for all database units and `int64_t`
for all area calculations. Refer to this [link](DatabaseMath.md) for a more
detailed writeup on the reasons why this approach is preferred. The only
place that the database distance units should appear in any program
should be in the user interface, as microns are easier for humans than DBUs.
