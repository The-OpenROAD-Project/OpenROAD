# Introduction

OpenDB is a design database to support tools for physical chip
design. It was originally developed by Athena Design Systems. Nefelus,
Inc acquired the rights to the code and open sourced it in 2019 to
support the DARPA OpenROAD project.

The structure of OpenDB is based on the Cadence Design Systems text
file formats LEF (library) and DEF (design) formats version 5.6.
OpenDB supports a binary file format to save and load the design much
faster than using LEF and DEF.

OpenDB is written in C++ 98 with standard library style iterators.
The classes are designed to be fast enough to base an application
on without having to copy them into application specific structures.


## Installation

```
git clone https://github.com/The-OpenROAD-Project/OpenDB.git
cd OpenDB
mkdir build
cd build
cmake ..
make
```

The original Athena code found in /zrouter is built using Makefiles.
See zrouter/src/BUILD/compilation_package_dependencies.

## Directory structure

```
include/opendb/db.h - public header for all database classes
src/db - private/internal database representations
src/lefin - LEF reader
src/lefout - LEF writer
src/defin - DEF reader
src/defout - DEF writer
```

## Database API

:bangbang: We are still working on documenting the APIs. 
We have over 1,800 objects and functions that we are still documenting (for both TCL and Python). 
**Contributions are very welcome in this effort**. Find starting points below.

### TCL
After building successfully, run OpenDB tcl shell using `./build/src/swig/tcl/opendbtcl`. An example usage:

```
set db [dbDatabase_create]
set lef_parser [new_lefin $db true]
set tech [lefin_createTech $lef_parser ./OpenDB/tests/data/gscl45nm.lef]
```

You can find examples on using the API from TCL under `tests/tcl/` directory.

The full set of the tcl commands exposed can be found under `./build/src/swig/tcl/opendb_wrapper.cpp`. Search for `SWIG_prefix`.


### Python
After building successfully, run a Python shell using `python3`. Load `opendbpy module using:

```
import importlib.util
spec = importlib.util.spec_from_file_location("opendbpy", "./build/src/swig/python/opendbpy.py")
odb = importlib.util.module_from_spec(spec)
spec.loader.exec_module(odb)

# use it as following
odb.[class_name]
```
You can find examples on using the API from Python under `tests/python/` directory.

The full set of the Python classes exposed can be found under `./build/src/swig/python/opendbpy.py`.

### C++

All public database classes are defined in `db.h`. These class
definitions provide all functions for examining and modifying the
database objects. The database is an object itself so multiple
database objects can exist simultaineously (no global state).

`dbTypes.h` defines types returned by database class member functions.

All database objects are in the `odb` namespace.

`dbChip`
`dbBlock`
`dbTech`
`dbLib`

All database objects have a 32bit object identifier accessed with the
dbObject::getOID base class member function that returns a
`uint`. This identifier is preserved across save/restores of the
database so it should be used to reference database object by data
structures instead of pointers if the reference lifetime is across
database save/restores. OIDs allow the database to have exactly the
same layout across save/restores.

The database distance units are **nanometers** and use the type `uint`.

## Tests
There are a set of regression tests in /tests.

```
./tests/regression-tcl.sh
./tests/regression-py.sh
```

## Database Internals

The internal description included here is paraphrased from Lukas van
Ginneken by James Cherry.

The database separates the implementation from the interface, and as a
result, each class becomes two classes, a public one and a private
one. For instance, `dbInst` has the public API functions, while class
`_dbInst` has the private data fields.

The objects allocated in dynamically resizable tables the
implementation of which is in `dbTable.hpp`. Each table consists of a
number of pages, each containing 128 objects. The table contains the
body of the struct, not a set of pointers. This eliminates most of the
pointer overhead while iteration is accomplished by stepping through
the table. Thus grouping these objects does not require a double
linked list and saves 16 bytes per object (at the cost of some table
overhead). Each object has an id, which is the index into the
table. The lowest 7 bits are the index in the page, while the higher
bits are the page number. Object id's are persistent when saving and
reading the datamodel to disk, even as pointer addresses may change.

Everything in the data model can be stored on disk and restored from
disk exactly the way it was. An extensive set of equality tests and
diff functions make it possible to check for even the smallest
deviation. The capability to save an exact copy of the state of the
system makes it possible to create a checkpoint. This is a necessary
capability for debugging complex systems.

The code follows the definition of LEF and DEF closely and reflects
many of the idiosyncrasies of LEF and DEF. The code defines many types
of objects to reflect LEF and DEF constructs although it sometimes
uses different terminology, for instance, the object to represent a
library cell is called dbMaster while the LEF keyword is MACRO.

The data model supports the EEQ and LEQ keywords, (electrically
equivalent and logically equivalent Masters) which could be used for
sizing. However, it does not support any logic function
representation. In general, there is very limited support for
synthesis specific information: no way to represent busses, no way to
represent logic function, very limited understanding of signal flow,
limited support of timing information, no support for high level
synthesis or test insertion.

The db represents routing as in DEF, representing a trace from point
to point with a given width. The layout for a net is stored in a class
named dbWire and it requires a special dbWireDecoder (which works like
an iterator) to unpack the data and another dbWireEncoder to pack
it. The data model does not support a region query and objects that
are in the same layer are scattered about the data model and are of
different classes.

This means that whatever tool is using the layout information will
have to build it's own data structures that are suitable to the layout
operations of that tool. For instance, the router, the extractor, and
the DRC engine would each have to build their unique data
structures. This encourages batch mode operation (route the whole
chip, extract the whole chip, run DRC on the whole chip).


# LICENSE

BSD 3-Clause License. See [LICENSE](LICENSE) file