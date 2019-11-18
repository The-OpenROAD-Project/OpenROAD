### Tool File Organization

Every tool follows the following file structure.

```
CMakelists.txt - add_subdirectory's src/CMakelists.txt
/src/ - sources and private headers
/src/CMakelists.txt
/include/<toolname>/ - exported headers
/test/
/Dockerfile
/Jenkinsfile
/jenkins/
```

OpenROAD repo

```
src/Main.cc
src/OpenROAD.cc - OpenROAD class functions
src/OpenROAD.i - top level swig, %includes tool swig files
src/OpenROAD.tcl - basic read/write lef/def/db commands
src/OpenROAD.hh - OpenROAD top level class, has instances of tools
```

Tools that are part of the OpenROAD repo because they are not a lot of code
or terribly useful without the rest of OpenROAD.
```
src/dbReadVerilog.* - Verilog reader/flattener
src/InitFloorplan.* - Initialize floorplan
src/StaDB/ - OpenSTA on OpenDB.
src/Resizer/ - gate resizer
```

Submodule repos (note these are NOT in src/module)

```
src/OpenDB
src/OpenSTA
src/replace
src/flute3
```

None of the tool repos have submodules. All submodules are owned by OpenROAD
so that there are not redundant source trees and compiles.

Each tool submodule cmake file builds a library that is linked by the
OpenROAD application. The tools should not define a `main()` function.

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
such as 'global_place_design' that do not create tool instances that
must be based to the commands. Definiting Tcl commands for a tool
class is fine for internals, but not for user visible
commands. Commands have an implicit argument of the current OpenROAD
class object. Functions to get individual tools from the OpenROAD
object can be defined.

### Initialization

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
Use swig to define internal functions to C++ functionality.

Tcl files can be included by encoding them in cmake into a string
that is evaluated at run time (See Resizer::init()).

### Test

Each "tool" has a /test directory containing a script to run "unit" tests.

No databases should be in tests. Read lef/def/verilog to make a database.

### Issues

Using Tcl wrappers for commands does not work in python without
rewritting them.  The only way to make both work is to build command
support (see OpenSTA/tcl/Util.tcl) in c++ and make exported swig
commands use it. OpenSTA has quite a bit of Tcl that would have to be
rewritten as c++. In the short term, the python version may be
hobbled.

Reporting (printing) standard. printf does not work in c++ code with
Tcl because the buffers are not flushed and the output of one can be
delayed. printf also does not support logging or redirection. OpenDB
and OpenSTA have separate solutions to this issue. We have to pick
a comment solution. I have not studied the OpenDB code.

### Builds

Checking out the OpenROAD repo with --recursive installs all of the
OpenRoad tools and their submodules.

  git clone --recusive https://github.com/The-OpenROAD-Project/OpenROAD.git
  cd OpenROAD
  mkdir build
  cd build
  cmake ..
  make
  
This builds the 'openroad' executable.

A stand-alone executable for one tool can be built by making a branch
specific to that tool. For example, there is a branch named "sta_only"
that builds the openroad executable that only includes OpenSTA running
on OpenDB. The "sta_only" tool is checked out and built exactly as
OpenRoad is built by specifying a branch during the git clone as show
below.

```
git clone --recursive --branch sta_only https://github.com/The-OpenROAD-Project/OpenROAD.git
```

In this example, the Resizer and its dependent submodule flute3 are
not installed in the source tree and are not compiled during builds.
It can be used to run unit tests that reside inside the tool
directory.

Currently this is supported by editing Resizer related calls in the
OpenROAD sources. As the sources in the develop/master branch change
the tool branch is rebased to follow them, keeping only the edits
necessary to remove other tools.

### Tool Flow

Verilog to DB (OpenDB, dbSTA/OpenSTA)
Init Floorplan (OpenDB)
ioPlacer (OpenDB)
PDN generation (OpenDB)
tapcell (OpenDB)
ioPlacer (OpenDB)
RePlAce (OpenDB, dbSTA/OpenSTA, flute3)
Resizer (OpenDB, dbSTA/OpenSTA, flute3)
OpenDP (OpenDB,  dbSTA/OpenSTA, flute3)
TritonCTS (OpenDB)
FRlefdef (OpenDB)
TritonRoute (OpenDB)
Final report (OpenDB, dbSTA/OpenSTA)
