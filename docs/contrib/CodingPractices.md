# Coding Practices

List of coding practices.

---
**NOTE**

This is a compilation of many idioms in openroad code that I consider
undesirable. Obviously other programmers have different opinions or they
would not be so pervasive. James Cherry 04/2020

---

## C++

### Practice #1

Don't comment out code. Remove it. git provides a complete history of
the code if you want to look backwards. Huge chunks of commented out
code that are stunningly common in student code makes it nearly
impossible to read.

FlexTa.cpp has 220 lines of code and 600 lines of commented out code.

### Practice #2

Don't use prefixes on function names or variables. That's what
namespaces are for.

``` cpp
namespace fr {
  class frConstraint
  class frLef58CutClassConstraint
  class frShortConstraint
  class frNonSufficientMetalConstraint
  class frOffGridConstraint
  class frMinEnclosedAreaConstraint
  class frMinStepConstraint
  class frMinimumcutConstraint
  class frAreaConstraint
  class frMinWidthConstraint
  class frLef58SpacingEndOfLineWithinEndToEndConstraint
  class frLef58SpacingEndOfLineWithinParallelEdgeConstraint
  class frLef58SpacingEndOfLineWithinMaxMinLengthConstraint
  class frLef58SpacingEndOfLineWithinConstraint
  class frLef58SpacingEndOfLineConstraint
}
```

### Practice #3

Namespaces should be all lower case and short. This is an example of a
poor choice: `namespace TritonCTS`

### Practice #4

Don't use `extern` on function definitions. It is pointless
in a world with prototypes.

``` cpp
namespace fr {
  extern frCoord getGCELLGRIDX();
  extern frCoord
  getGCELLGRIDY();
  extern frCoord getGCELLOFFSETX();
  extern frCoord
  getGCELLOFFSETY();
}
```

### Practice #5

Don't use prefixes on file names. That's what directories are for.

``` shell
frDRC.h frDRC_init.cpp frDRC_main.cpp frDRC_setup.cpp frDRC_util.cpp
```

### Practice #6

Don't name variables theThingy, curThingy or myThingy. It is just
distracting extraneous verbage. Just use thingy.

``` cpp
float currXSize;
float currYSize;
float currArea;
float currWS;
float currWL;
float currWLnoWts;
```

### Practice #7

Do not use global varaibles. All state should be inside of classes.
Global variables make multi-threading next to impossible and preclude
having multiple copies of a tool running in the same process. The only
global variable in `openroad` should be the singleton that tcl commands
reference.

``` cpp
extern std::string DEF_FILE;
extern std::string GUIDE_FILE;
extern std::string OUTGUIDE_FILE;
extern std::string LEF_FILE;
extern std::string OUTTA_FILE;
extern std::string OUT_FILE;
extern std::string DBPROCESSNODE;
extern std::string OUT_MAZE_FILE;
extern std::string DRC_RPT_FILE;
extern int MAX_THREADS ;
extern int VERBOSE ;
extern int BOTTOM_ROUTING_LAYER;
extern bool ALLOW_PIN_AS_FEEDTHROUGH;
extern bool USENONPREFTRACKS;
extern bool USEMINSPACING_OBS;
extern bool RESERVE_VIA_ACCESS;
extern bool ENABLE_BOUNDARY_MAR_FIX;
```

### Practice #8

Do not use strings (names) to refer to database or sta objects except in
user interface code. DEF, SDC, and verilog all use different names for
netlist instances and nets so the names will not always match.

### Practice #9

Do not use continue. Wrap the body in an if instead.

``` cpp
// instead of
for(dbInst* inst : block->getInsts() ) {
  // Skip for standard cells
  if (inst->getBBox()->getDY() <= cellHeight) { continue; }
  // code
}
// use
for(dbInst* inst : block->getInsts() ){
  // Skip for standard cells
  if (inst->getBBox()->getDY() > cellHeight) {
    // code
  }
}
```

### Practice #10

Don't put magic numbers in the code. Use a variable with a name that
captures the intent. Document the units if they exist.

examples of unnamed magic numbers:

``` cpp
referenceHpwl_= 446000000;
coeffV = 1.36;
coeffV = 1.2;
double nearest_dist = 99999999999;
if (dist < rowHeight * 2) {}
for(int i = 9; i > -1; i--) {}
if(design_util > 0.6 || num_fixed_nodes > 0) div = 1;
avail_region_area += (theRect->xUR - theRect->xLL - (int)theRect->xUR % 200 + (int)t  heRect->xLL % 200 - 200) * (theRect->yUR - theRect->yLL - (int)theRect->yUR % 2000 + (int)theRect->yLL % 2000 - 2000);
```

### Practice #11

Don't copy code fragments. Write functions.

``` cpp
// 10x
int x_pos = (int)floor(theCell->x_coord / wsite + 0.5);
// 15x
int y_pos = (int)floor(y_coord / rowHeight + 0.5);

// This
nets[newnetID]->netIDorg = netID;
nets[newnetID]->numPins = numPins;
nets[newnetID]->deg = pinInd;
nets[newnetID]->pinX = (short *)malloc(pinInd* sizeof(short));
nets[newnetID]->pinY = (short *)malloc(pinInd* sizeof(short));
nets[newnetID]->pinL = (short *)malloc(pinInd* sizeof(short));
nets[newnetID]->alpha = alpha;

// Should factor out the array lookup.
Net *net = nets[newnetID];
net->netIDorg = netID;
net->numPins = numPins;
net->deg = pinInd;
net->pinX = (short*)malloc(pinInd* sizeof(short));
net->pinY = (short *)malloc(pinInd* sizeof(short));
net->pinL = (short *)malloc(pinInd* sizeof(short));
net->alpha = alpha;

// Same here:
if (grid[j][k].group != UINT_MAX) {
  if (grid[j][k].isValid) {
    if (groups[grid[j][k].group].name == theGroup->name)
      area += wsite * rowHeight;
  }
}
```

### Practice #12

Don't use logical operators to test for null pointers.

``` cpp
if (!net) {
  // code
}

// should be
if (net != nullptr) {
  // code
}
```

### Practice #13

Don't use malloc. Use new. We are writting C++, not C.

### Practice #14

Don't use C style arrays. There is no bounds checks for them so they
invite subtle memory errors to unwitting programmers that fail to use
valgrind. Use std::vector or std::array.

### Practice #15

Break long functions into smaller ones, preferably that fit on one
screen.

- 162 lines void DBWrapper::initNetlist()
- 246 lines static vector<pair<Partition, Partition>> GetPart()
- 263 lines void MacroCircuit::FillVertexEdge()

### Practice #16

Don't reinvent functions like round, floor, abs, min, max. Use the std
versions.

``` cpp
int size_x = (int)floor(theCell->width / wsite + 0.5);
```

### Practice #17

Don't use C stdlib.h abs, fabs or fabsf. They fail miserably if the
wrong arg type is passed to them. Use std::abs.

### Practice #18

Fold code common to multiple loops into the same loop. Each of these
functions loops over every instance like this:

``` cpp
legal &= row_check(log);
legal &= site_check(log);
for(int i = 0; i < cells.size(); i++) {
  cell* theCell = &cells[i];
  legal &= power_line_check(log);
  legal &= edge_check(log);
  legal &= placed_check(log);
  legal &= overlap_check(log);
}
// with this loop
for(int i = 0; i < cells.size(); i++) {
  cell* theCell = &cells[i];
}
```

Instead make one pass over the instances doing each check.

### Practice #19

Don't use == true, or == false. Boolean expressions already have a
value of true or false.

``` cpp
if(found.first == true) {
  // code
}
// is simply
if(found.first) {
  // code
}
// and
if(found.first == false) {
  // code
}
// is simply
if(!found.first) {
 // code
}
```

### Practice #20

Don't nest if statements. Use && on the clauses instead.

``` cpp
if(grid[j][k].group != UINT_MAX)
  if(grid[j][k].isValid == true)
    if(groups[grid[j][k].group].name == theGroup->name)
```

is simply

``` cpp
if(grid[j][k].group != UINT_MAX
   && grid[j][k].isValid
   && groups[grid[j][k].group].name == theGroup->name)
```

### Practice #21

Don't call return at the end of a function that does not return a
value.

### Practice #22

Don't use <>'s to include anything but system headers. Your
project's headers should NEVER be in <>'s. -
<https://gcc.gnu.org/onlinedocs/cpp/Include-Syntax.html> -
<https://stackoverflow.com/questions/21593/what-is-the-difference-between-include-filename-and-include-filename>

These are all wrong:

``` cpp
#include <opendb/db.h>
#include <ABKCommon/uofm_alloc.h>
#include <OpenSTA/liberty/Liberty.hh>
#include <opendb/db.h>
#include <opendb/dbTypes.h>
#include <opendb/defin.h>
#include <opendb/defout.h>
#include <opendb/lefin.h>
```

### Practice #23

Don't make "include the kitchen sink" headers and include them in
every source file. This is convenient (lazy) but slows the builds down
for everyone. Make each source file include just the headers it actually
needs.

``` cpp
// Types.hpp
#include <OpenSTA/liberty/Liberty.hh>
#include <opendb/db.h>
#include <opendb/dbTypes.h>
// It should be obvious that every source file is not reading def.
#include <opendb/defin.h>
// or writing it.
#include <opendb/defout.h>
#include <opendb/lefin.h>
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
```

Note this example also incorrectly uses <>'s around openroad headers.

Header files should only include files to support the header. Include
files necessary for code in the code file, not the header.

In the example below NONE of the system files listed are necessary for
the header file.

``` cpp
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

unsigned num_nets = 1000;
unsigned num_terminals = 64;
unsigned verbose = 0;
float alpha1 = 1;
float alpha2 = 0.45;
float alpha3 = 0;
float alpha4 = 0;
float margin = 1.1;
unsigned seed = 0;
unsigned root_idx = 0;
unsigned dist = 2;
float beta = 1.4;
bool runOneNet = false;
unsigned net_num = 0;
```

### Practice #24

Use class declarations if you are only refering to object by pointer
instead of including their complete class definition. This can vastly
reduce the code the compiler has to process.

``` cpp
class Network;
// instead of
#include "Network.hh"
```

### Practice #25

Use pragma once instead of #define to protect headers from being read
more than once. The #define symbol has to be unique, which is difficult
to guarantee.

``` cpp
// Instead of:
#ifndef __MACRO_PLACER_HASH_UTIL__
#define __MACRO_PLACER_HASH_UTIL__
#endif
// use
#pragma once
```

### Practice #26

Don't put "using namespace" inside a function. It makes no sense what
so ever but I have seen some very confused programmers do this far too
many times.

### Practice #27

Don't nest namespaces. We don't have enough code to justify that
complication.

### Practice #28

Don't use `using namespace` It is just asking for conflicts
and doesn't explicity declare what in the namespace is being used. Use
`using namespace::symbol;` instead. And especially NEVER
EVER EVER `using namespace std`. It is HUGE.

The following is especially confused because it is trying to "use" the
symbols in code that is already in the MacroPlace namespace.

``` cpp
using namespace MacroPlace;

namespace MacroPlace { }
```

### Practice #29

Use `nullptr` instead of `NULL`. This is the C++
approved version of the ancient C `#define`.

### Practice #30

Use range iteration. C++ iterators are ugly and verbose.

``` cpp
// Instead of
odb::dbSet::iterator nIter;
for (nIter = nets.begin(); nIter != nets.end(); ++nIter) {
  odb::dbNet* currNet = *nIter;
  // code
}
// use
for (odb::dbNet* currNet : nets) {
  // code
}
```

### Practice #34

Don't use end of line comments unless they are very short. Don't
assume that the person reading your code has a 60" monitor.

``` cpp
for (int x = firstTile._x; x <= lastTile._x; x++) { // Setting capacities of edges completely inside the adjust region according the percentage of reduction
  // code
}
```

### Practice #35

Don't std::pow for powers of 2 or for decimal constants.

``` cpp
// This
double newCapPerSqr = (_options->getCapPerSqr() * std::pow(10.0, -12));
// Should be
double newCapPerSqr = _options->getCapPerSqr() * 1E-12;

// This
unsigned numberOfTopologies = std::pow(2, numberOfNodes);
// Should be
unsigned numberOfTopologies = 1 << numberOfNodes;
```

## Git

### Practice #31

Don't put /'s in `.gitignore` directory names.
`test/`

### Practice #32

Don't put file names in `.gitignore` ignored directories.
`test/results` `test/results/diffs`

### Practice #33

Don't list compile artifacts in `.gitignore` They all end
up in the build directory so each file type does not have to appear in
`.gitignore`.

All of the following is nonsense that has propagated faster than covid
in student code:

#### Compiled Object files

`*.slo *.lo *.o *.obj`

#### Precompiled Headers

`*.gch *.pch`

#### Compiled Dynamic libraries

`*.so *.dylib *.dll`

#### Fortran module files

`*.mod *.smod`

#### Compiled Static libraries

`*.lai *.la *.a *.lib`

## CMake

### Practice #35

Don't change compile flags in `cmake` files. These are set at the top
level and should not be overriden.

``` cmake
set(CMAKE_CXX_FLAGS "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
```

### Practice #36

Don't put /'s in CMake directory names. CMake knows they are directories.

``` cmake
target_include_directories( ABKCommon PUBLIC ${ABKCOMMON_HOME} src/ )
```

### Practice #37

Don't use glob. Explictly list the files in a group.

``` cmake
# Instead of
file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# should be
list(REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/Main.cpp)
list(REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/Parameters.h)
list(REMOVE_ITEM SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/Parameters.cpp)
```
