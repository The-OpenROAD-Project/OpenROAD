### Tool Philosophy

OpenROAD is a tool to build a chip from a synthesized netlist to a
physical design for manufacturing.

The unifying principle behind the design of OpenROAD is for all of the
tools to reside in one tool, with one process, and one database.  All
tools in the flow should use Tcl commands exclusively to control them
instead of external "configuration files".  File based communication
between tools and forking processes is strongly discouraged. This
architecture streamlines the construction of a flexible tool flow and
minimizes the overhead of invoking each tool in the flow.

### Tool File Organization

Every tool follows the following file structure.

```
CMakelists.txt - add_subdirectory's src/CMakelists.txt
src/ - sources and private headers
src/CMakelists.txt
include/<toolname>/ - exported headers
test/
test/regression
```

OpenROAD repository

```
CMakeLists.txt - top level cmake file
src/Main.cc
src/OpenROAD.cc - OpenROAD class functions
src/OpenROAD.i - top level swig, %includes tool swig files
src/OpenROAD.tcl - basic read/write lef/def/db commands
include/openroad/OpenRoad.hh - OpenROAD top level class, has instances of tools
include/openroad/Error.hh - Error reporting API

```

Submodule repos in /src (note these are NOT in src/module)

```
OpenDB
OpenSTA
replace
ioPlacer
FastRoute
TritonMacroPlace
OpenRCX
flute3
eigen
```

Submodules that are shared by multiple tools are owned by OpenROAD
so that there are not redundant source trees and compiles.

Each tool submodule cmake file builds a library that is linked by the
OpenROAD application. The tools should not define a `main()` function.
If the tool is tcl only and has no c++ code it does not need to have
a cmake file.

None of the tools have commands to read or write LEF, DEF, Verilog or
database files.  These functions are all provided by the OpenROAD
framework for consistency.

Tools should package all state in a single class. An instance of each
tool class resides in the top level OpenROAD object. This allows
multiple tools to exist at the same time. If any tool keeps state in
global variables (even static) only one tool can exist at a time.
Many of the tools being integrated were not built with this goal in
mind and will only work on one design at a time. Eventually all of the
tools should be upgraded to remove this deficiency as they are
re-written to work in the OpenROAD framework.

Each tool should use a unique namespace for all of its code.  The same
namespace should be used for any Tcl commands.  Internal Tcl commands
stay inside the namespace, user visible Tcl commands will be exported
to the global namespace. User commands should be simple Tcl commands
such as 'global_placement' that do not create tool instances that must
be based to the commands. Defining Tcl commands for a tool class is
fine for internals, but not for user visible commands. Commands have
an implicit argument of the current OpenROAD class object. Functions
to get individual tools from the OpenROAD object can be defined.

### Initialization (c++ tools only)

The OpenRoad class only has pointers to each tools with functions to
get each tool.  Each tool has (at a minimum) a function to make an
instance of the tool class, and an initialization function that is
called after all of the tools have been made, and a funtion to delete
the tool. This small header does NOT include the class definition for
the tool so that the OpenRoad framework does not have to know anything
about the tool internals or include a gigantic header file.

`MakeTool.hh` defines the following:

```
Tool *makeTool();
void initTool(OpenRoad *openroad);
void deleteTool(Tool *tool);

```

The OpenRoad::init() function calls all of the makeTool functions and
then all of the initTool() functions. The init functions are called
from the bottom of the tool dependences. Each init function grabs the
state it needs out of the OpenRoad instance.

### Commands

Tools should provide Tcl commands to control them. Tcl object based
tool interfaces are not user friendly. Define Tcl procedures that take
keyword arguments that reference the OpenRoad object to get tool
state.  OpenSTA has Tcl utilities to parse keyword arguements
(sta::parse_keyword_args). See OpenSTA/tcl/*.tcl for examples.
Use swig to define internal functions to C++ functionality.p

Tcl files can be included by encoding them in cmake into a string
that is evaluated at run time (See Resizer::init()).

### Errors

Tools should report errors to the user using the `ord::error` function
defined in `include/openroad/Error.hh`. `ord::error` throws
`ord::Exception`.  The variables `ord::exit_on_error` and
`ord::file_continue_on_error` control how the error is handled.  If
`ord::exit_on_error` is `true` OpenROAD reports the error and exits.
If the error is encountered while reading a file with the `source` or
`read_sdc` commands and `ord::file_continue_on_error` is `false` no
other commands are read from the file. The default values of both
variables is `false`.

### Test

Each "tool" has a /test directory containing a script nameed
"regression" to run "unit" tests. With no arguments it should run
default unit tests.

No database files should be in tests. Read LEF/DEF/Verilog to make a
database.

The regression script should not depend on the current working
directory.  It should be able to be run from any directory. Use
filenames relative to the script name rather the the current working
directory.

Regression scripts should print a consise summary of test failures.
The regression script should return an exit code of zero if there are
no errors and 1 if there are errors.  The script should **not** print
thousands of lines of internal tool info.

Regression scripts should pass the `-no_init` option to openroad so that a
user's init file is not sourced before the tests runs.

Regression scripts should add output files or directories to
`.gitignore` so that running does note leave the source repository
"dirty".

### Builds

Checking out the OpenROAD repo with --recursive installs all of the
OpenRoad tools and their submodules.

```
git clone --recusive https://github.com/The-OpenROAD-Project/OpenROAD.git
cd OpenROAD
mkdir build
cd build
cmake ..
make
```

All tools build using cmake and must have a CMakeLists.txt file in
their tool directory.

This builds the `openroad` executable in `/build`.

Note that removing submodules from a repo when moving it into OpenROAD
is less than obvious.  Here are the steps:

```
git submodule deinit <path_to_submodule>
git rm <path_to_submodule>
git commit-m "Removed submodule "
rm -rf .git/modules/<path_to_submodule>
```

Tools should compile with no compile warnings in gcc or clang with -Wall.

### Tool Work Flow

To work on one of the tools inside OpenROAD when it is a submodule
requires updating the OpenROAD repo to integrate your changes.
Submodules point to a specific version (hash) of the submodule repo
and do not automatically track changes to the submodule repo.

Work on OpenROAD should be done in the `openroad` branch.  Stable
commits on the `openroad` branch are periodically pushed to the
`master` branch for public consumption.

To make changes to a submodule, first check out a branch of the submodule
(git clone --recursive does not check out a branch, just a specific commit).

```
cd src/<tool>
git checkout <branch>
```

`<branch>` is the branch used for development of the tool when it is inside
OpenROAD. The convention is for <branch> to be named 'openroad'.

After making changes inside the tool source tree, stage and commit
them to the tool repo and push them to the remote repo.

```
git add ...
git commit -m "massive improvement"
git push
```

If instead you have done development in a different branch or source tree,
merge those changes into the branch used for OpenROAD.

Once the changes are in the OpenROAD submodule source tree it will show
them as a diff in the hash for the directory.

```
cd openroad
git stage <tool_submodule_dir>
git commit -m "merge tool massive improvement"
git push
```

### Example of Adding a Tool to OpenRoad

The branch "add_tool" illustrates how to add a tool to OpenRoad.  Use
`git checkout add_tool` to checkout the branch. To see the changes
between OpenRoad with and without Tool use `git diff master`.

This adds a directory OpenRoad/src/tool that illustrates a tool named "Tool"
that uses the file structure described and defines a command to run the tool
with keyword and flag arguments as illustrated below:

```
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

### Documentation

Tool commands should be documented in the top level OpenROAD README.md file.
Detailed documentation should be the tool/README.md file.

### Tool Flow

1. Verilog to DB (dbSTA)
2. Init Floorplan (OpenROAD)
3. I/O placement (ioPlacer)
4. PDN generation (pdngen
5. Tapcell and Welltie insertion (tapcell with LEF/DEF)
6. I/O placement (ioPlacer)
7. Global placement (RePlAce)
8. Gate Resizing and buffering (Resizer)
9. Detailed placement (OpenDP)
10. Clock Tree Synthesis (TritonCTS)
11. Repair Hold Violations (Resizer)
12. Global route (FastRoute)
13. Detailed route (TritonRoute)n
14. Final timing/power report (OpenSTA)

### Tool Checklist

Tools should make every attempt to minimize external dependencies.
Linking libraries other than those currently in use complicates the
builds and sacrifices the portability of OpenROAD. OpenROAD should be
portable to many different compiler/operating system versions and
dependencies make this vastly more complicated.

OpenROAD submodules reference tool `openroad` branch head.
No git `develop`, `openroad_app`, or `openroad_build` branches.

Submodules used by more than one tool belong in /src, not duplicated
in each tool repo.

CMakeLists.txt does not use
 add_compile_options
 include_directories
 link_directories
 link_libraries
Use target_ versions instead.
See https://gist.github.com/mbinna/c61dbb39bca0e4fb7d1f73b0d66a4fd1

CMakeLists.txt does not use glob.
Use explicit lists of source files and headers instead.

CMakeLists.txt does not define CFLAGS 
 CMAKE_CXX_FLAGS
 CMAKE_CXX_FLAGS_DEBUG
 CMAKE_CXX_FLAGS_RELEASE
Let the top level and defaults control these.

No main.cpp or main procedure.

No compiler warnings for gcc or clang with optimization enabled.

Does not call flute::readLUT (called once by OpenRoad).

Tcl command(s) documented in top level README.md in flow order.

Command line tool documentation in tool README.

Conforms to Tcl command naming standards (no camel case).

Does not read configuration files. 
Use command arguments or support commands.

.clang-format at tool root directory to aid foreign programmers.

No jenkins/, Jenkinsfile, Dockerfile in tool directory.

regression script named "test/regression" with no arguments that runs
tests. Not tests/regression-tcl.sh, not test/run_tests.py etc.

regression script should run independent of current directory.
For example, ../test/regression should work.

regression should only print test results or summary, not belch 1000s
of lines of output.

Test scripts use OpenROAD tcl commands (not itcl, not internal accessors).

regression script should only write files in a directory that is in
the tool's .gitignore so the hierarchy does not have modified files in
it as a result or running the regressions.

Regressions report no memory errors with valgrind (stretch goal).

Regressions report no memory leaks with valgrind (difficult).

###

James Cherry, Dec 2019
