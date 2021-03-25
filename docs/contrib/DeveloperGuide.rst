Developer Guide
===============

OpenROAD is a tool to build a chip from a synthesized netlist to a
physical design for manufacturing.

The unifying principle behind the design of OpenROAD is for all of the
tools to reside in one tool, with one process, and one database. All
tools in the flow should use Tcl commands exclusively to control them
instead of external “configuration files”. File based communication
between tools and forking processes is strongly discouraged. This
architecture streamlines the construction of a flexible tool flow and
minimizes the overhead of invoking each tool in the flow.

Tool File Organization
----------------------

Every tool follows the following file structure.

::

   CMakelists.txt - add_subdirectory's src/CMakelists.txt
   src/ - sources and private headers
   src/CMakelists.txt
   include/<tool_tla>/ - public headers
   test/
   test/regression

OpenROAD repository

::

   CMakeLists.txt - top level cmake file
   src/Main.cc
   src/OpenROAD.cc - OpenROAD class functions
   src/OpenROAD.i - top level swig, %includes tool swig files
   src/OpenROAD.tcl - basic read/write lef/def/db commands
   src/OpenROAD.hh - OpenROAD top level class, has instances of tools

Submodules are only for tools that are not a part of OpenSTA and are
externally owned or maintained. Submodule repos in /src (note these
are NOT in src/module). The only submodule currently used in OpenROAD
is OpenSTA.

Libraries that are shared by multiple tools are owned by OpenROAD so
that there are not redundant source trees and compiles.

Each tool cmake file builds a library that is linked by the OpenROAD
application. The tools should not define a ``main()`` function.  If
the tool is tcl only and has no c++ code it does not need to have a
cmake file.

None of the tools should have commands to read or write LEF, DEF,
Verilog or database files. These functions are all provided by the
OpenROAD framework for consistency.

Tools should package all state in a single class. An instance of each
tool class resides in the top level OpenROAD object. This allows
multiple tools to exist at the same time. If any tool keeps state in
global variables (even static) only one tool can exist at a time. Many
of the tools being integrated were not built with this goal in mind and
will only work on one design at a time. Eventually all of the tools
should be upgraded to remove this deficiency as they are re-written to
work in the OpenROAD framework.

Each tool should use a unique namespace for all of its code. The same
namespace should be used for Tcl commands. Internal Tcl commands stay
inside the namespace, user visible Tcl commands will be exported to
the global namespace. User commands should be simple Tcl commands such
as ‘global_place_design’ that do not create tool instances that must
be based to the commands. Defining Tcl commands for a tool class is
fine for internals, but not for user visible commands. Commands have
an implicit argument of the current OpenROAD class object. Functions
to get individual tools from the OpenROAD object can be defined.

Initialization (c++ tools only)
-------------------------------

The OpenRoad class only has pointers to each tool with accessor
functions to get each tool. Each tool has (at a minimum) a function to
make an instance of the tool class, and an initialization function
that is called after all of the tools have been made, and a funtion to
delete the tool. This small header does NOT include the class
definition for the tool so that the OpenRoad framework does not have
to know anything about the tool internals or include a gigantic header
file.

``MakeTool.hh`` defines the following:

::

   Tool *makeTool();
   void initTool(OpenRoad *openroad);
   void deleteTool(Tool *tool);

The OpenRoad::init() function calls all of the makeTool functions and
then all of the initTool() functions. The init functions are called from
the bottom of the tool dependences. Each init function grabs the state
it needs out of the OpenRoad instance.

Commands
--------

Tools should provide Tcl commands to control them. Using so called
"configuration files" is poor design. Tcl object based tool
interfaces are not user friendly. Define Tcl procedures that take
keyword arguments that reference the OpenRoad object to get tool
state.  OpenSTA has Tcl utilities to parse keyword arguements
(sta::parse_keyword_args). See `OpenSTA/tcl/*.tcl` for examples. Use
swig to define internal functions to C++ functionality.

Tcl files can be included by encoding them in cmake into a string that
is evaluated at run time (See Resizer::init()).

Test
----

Each “tool” has a /test directory containing a script nameed
“regression” to run “unit” tests. With no arguments it should run
default unit tests.

No database files should be in tests. Read LEF/DEF/Verilog to make a
database.

The regression script should not depend on the current working
directory. It should be able to be run from any directory. Use filenames
relative to the script name rather the the current working directory.

Regression scripts should print a consise summary of test failures. The
regression script should return an exit code of zero if there are no
errors and 1 if there are errors. The script should **not** print
thousands of lines of internal tool info.

Unit tests should use small data sets and run in at most a few
seconds.  Unit tests should take advantage of public common technology
files (LEF, liberty etc) in the top level test directory by adding a
symbolic link to them in the tool/test directory.

Builds
------

Checking out the OpenROAD repo with –recursive installs all of the
OpenRoad tools and their submodules.

::

   git clone --recusive https://github.com/The-OpenROAD-Project/OpenROAD.git
   cd OpenROAD
   mkdir build
   cd build
   cmake ..
   make

All tools build using cmake and must have a CMakeLists.txt file in their
tool directory.

This builds the ‘openroad’ executable in /build.

Note that removing submodules from a repo when moving it into OpenROAD
is less than obvious. Here are the steps:

::

   git submodule deinit <path_to_submodule>
   git rm <path_to_submodule>
   git commit-m "Removed submodule "
   rm -rf .git/modules/<path_to_submodule>

Tool Work Flow
--------------

Work on OpenROAD should be done in the private repository.  A separate
public repository is used for user access.  The public repository is
updated periodically from the private repository when regression tests
pass and the code is considered stable.

Work on OpenROAD should be done on a branch in the private repository.

::

   git switch -c <branch>

After making changes inside the tool source tree, stage and commit them
and push them to the remote repo.

::

   git add ...
   git commit -m "massive improvement"
   git push

Check the branch list on the OpenROAD github page to see if the branch passes
Jenkins. Once it does, submit a pull request to merge it. Once the pull request
is approved, merge the branch and delete it.

Example of Adding a Tool to OpenRoad
------------------------------------

The patch file docs/misc/AddTool.patch illustrates how to add a tool
to OpenRoad. Use ``patch -p 1 < docs/misc/AddTool.patch`` to apply the
patch. To see the changes between OpenRoad with and without Tool use
``git diff master``.

This adds a directory OpenRoad/src/tool that illustrates a tool named
“Tool” that uses the file structure described and defines a command to
run the tool with keyword and flag arguments as illustrated below:

::

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

Documentation
-------------

Tool commands should be documented in the top level OpenROAD README.md
file. More detailed documentation can be placed in tool/README.md file.

Tool Flow
---------

1.  Verilog to DB (dbSTA)
2.  Init Floorplan (OpenROAD)
3.  I/O placement (ioPlacer)
4.  PDN generation (pdngen
5.  Tapcell and Welltie insertion (tapcell with LEF/DEF)
6.  I/O placement (ioPlacer)
7.  Global placement (RePlAce)
8.  Gate Resizing and buffering (Resizer)
9.  Detailed placement (OpenDP)
10. Clock Tree Synthesis (TritonCTS)
11. Repair Hold Violations (Resizer)
12. Global route (FastRoute)
13. Detailed route (TritonRoute)
14. Final timing/power report (OpenSTA)

Tool Checklist
--------------

-  OpenROAD submodules reference tool ``openroad`` branch head
-  No ``develop``, ``openroad_app``, ``openroad_build`` branches.
-  CMakeLists.txt does not use glob.
   https://gist.github.com/mbinna/c61dbb39bca0e4fb7d1f73b0d66a4fd1
-  No main.cpp or main procedure.
-  No compiler warnings for gcc or clang with optimization enabled.
-  Tcl command(s) documented in top level README.md in flow order.
-  Conforms to Tcl command naming standards (no camel case).
-  Does not read configuration files.
-  Use command arguments or support commands.
-  ``.clang-format`` at tool root directory to aid foreign programmers.
-  No jenkins/, Jenkinsfile, Dockerfile in tool directory.
-  regression script named “test/regression” with default argument that
   runs unit tests. Not tests/regression-tcl.sh, not test/run_tests.py etc.
-  Regression runs independent of current directory.
-  Regression only prints test results or summary, does not belch 1000s
   of lines of output.
-  Test scripts use OpenROAD tcl commands (not itcl, not internal
   accessors).
-  Regressions report no memory errors with valgrind.
-  Regressions report no memory leaks with valgrind (difficult).

James Cherry, Dec 2019
