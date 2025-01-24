# OpenDB

The OpenDB (`odb`) module in OpenROAD is a design database to support tools for physical
chip design. It was originally developed by Athena Design Systems.
Nefelus, Inc. acquired the rights to the code and open-sourced it with BSD-3 license
in 2019 to support the DARPA OpenROAD project.

The structure of OpenDB is based on the text file formats LEF
(library) and DEF (design) formats version 5.6.  OpenDB supports a
binary file format to save and load the design much faster than using
LEF and DEF.

OpenDB is written in C++ 98 with standard library style iterators.
The classes are designed to be fast enough to base an application on without
having to copy them into application-specific structures.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

## Directory structure

```
include/odb/db.h - public header for all database classes
src/db - private/internal database representations
src/lefin - LEF reader
src/lefout - LEF writer
src/defin - DEF reader
src/defout - DEF writer
```

## Database API

We are still working on documenting the APIs.  We have over 1,800 objects
and functions that we are still documenting (for both TCL and Python).
**Contributions are very welcome in this effort**. Find starting points below.

## Python

After building successfully, run `openroad -python` to enable the Python
interpreter. You can find examples on using the API from Python under
`test/python/` directory.

To list the full set of the Python classes exposed run `openroad -python`
then:
```
import openroad
import odb
print(', '.join(dir(openroad)))
print(', '.join(dir(odb)))
```

## C++

All public database classes are defined in `db.h`. These class definitions
provide all functions for examining and modifying the database objects. The
database is an object itself, so multiple database objects can exist
simultaneously (no global state).

`dbTypes.h` defines types returned by database class member functions.

All database objects are in the `odb` namespace.

-   `dbChip`
-   `dbBlock`
-   `dbTech`
-   `dbLib`

All database objects have a 32bit object identifier accessed with the
`dbObject::getOID` base class member function that returns a `uint`. This
identifier is preserved across save/restores of the database so it should
be used to reference database object by data structures instead of pointers
if the reference lifetime is across database save/restores. OIDs allow the
database to have exactly the same layout across save/restores.

The database distance units are **nanometers** and use the type `uint`.

### Create Physical Cluster

Description TBC.

```tcl
create_physical_cluster cluster_name
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `cluster_name` | Name of cluster. |


### Create Child Physical Clusters

Description TBC.

```tcl
create_child_physical_clusters 
    [-top_module]
or 
create_child_physical_clusters 
    [-modinst path] 
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `top_module` | TBC. |
| `-modinst` | TBC. |

### Set NDR Layer Rule

Description TBC.

```tcl
set_ndr_layer_rule  
    tech
    ndr
    layerName
    input
    isSpacing
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `tech` | TBC. |
| `ndr` | TBC. |
| `values` | TBC. |
| `isSpacing` | TBC. |

### Set NDR Rules

Description TBC.

```tcl
set_ndr_rules
    tech
    ndr
    values
    isSpacing
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `tech` | TBC. |
| `ndr` | TBC. |
| `layerName` | TBC. |
| `input` | TBC. |

### Create NDR

Description TBC.

```tcl
create_ndr
    -name name
    [-spacing val]
    [-width val]
    [-via val]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-name` | TBC. |
| `-spacing` | TBC. |
| `-width` | TBC. |
| `-via` | TBC. |

### Create Voltage Domain

Description TBC.

```tcl
create_voltage_domain
    domain_name
    -area {llx lly urx ury}
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-domain_name` | TBC. |
| `-area` | TBC. |

### Delete Physical Cluster

Description TBC.

```tcl
delete_physical_cluster cluster_name
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `cluster_name` | TBC. |

### Delete Voltage Domain

Description TBC.

```tcl
delete_voltage_domain domain_name
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `domain_name` | TBC. |

### Assign Power Net

Description TBC.

```tcl
assign_power_net 
    -domain domain_name
    -net snet_name
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-domain_name` | TBC. |
| `-net` | TBC. |

### Assign Ground Net

Description TBC.

```tcl
assign_ground_net
    -domain domain_name
    -net snet_name
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-domain_name` | TBC. |
| `-net` | TBC. |

### Add to Physical Cluster

Description TBC.

```tcl
add_to_physical_cluster
    [-modinst path]
    cluster_name
or 
add_to_physical_cluster
    [-inst inst_name]
    cluster_name
or
add_to_physical_cluster
    [-physical_cluster cluster_name]
    cluster_name
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-modinst` | TBC. |
| `-inst` | TBC. |
| `-physical_cluster` | TBC. |
| `cluster_name` | TBC. |

### Remove From Physical Cluster

Description TBC.

```tcl
remove_from_physical_cluster
    [-parent_module module_name]
    [-modinst modinst_name]
    cluster_name
or
remove_from_physical_cluster
    [-inst inst_name]
    cluster_name
or
remove_from_physical_cluster
    [-physical_cluster cluster_name]
    cluster_name
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-parent_module` | TBC. |
| `-modinst` | TBC. |
| `-inst` | TBC. |
| `-physical_cluster` | TBC. |
| `-cluster_name` | TBC. |

### Report Physical Clusters

Description TBC.

```tcl
report_physical_clusters
```

### Report Voltage Domains

Description TBC.

```tcl
report_voltage_domains
```

### Report Group

Description TBC.

```tcl
report_group group
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `group` | TBC. |

### Write Guides

This command writes global routing guides, which can be used as input 
for global routing.

Example: `write_guides route.guide`.

```tcl
write_guides file_name
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `file_name` | Guide file name. |

### Write Macro Placement

This command writes macro placement.

```tcl
write_macro_placement file_name
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `file_name` | Macro placement file name. |

### Design Is Routed

This command checks if the design is completely routed.

```tcl
design_is_routed [-verbose]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `verbose` | Flag that allow the command to show all the nets that are not routed. |


### Replace Design

This command swaps a hierarchical module with another module.
Two modules must have identical number of ports and port names must match.
Functional equivalence is not required.
New module is not allowed to have multiple levels of hierarchy for now.
Newly instantiated module is uniquified.

```tcl
replace_design instance_name module_name
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `instance_name` | Name of a hierarchical instance for which the module swap needs to happen.  For example, 'l1/l2/U3' |
| `module_name`   | Name of a new module that needs to be swapped in.  |


## Example scripts

After building successfully, run OpenDB Tcl shell using
`../../build/src/odb/src/swig/tcl/odbtcl`. An example usage:

```
set db [dbDatabase_create]
set lef_parser [new_lefin $db true]
set tech [lefin_createTech $lef_parser ./src/odb/test/data/gscl45nm.lef]
```

You can find examples on using the API from Tcl under `test/tcl/` directory.

The full set of the Tcl commands exposed can be found under
`./build/src/swig/tcl/opendb_wrapper.cpp`. Search for `SWIG_prefix`.

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests). 

Simply run the following script: 

```shell
./test/regression
```

## Database Internals

The internal description included here is paraphrased from Lukas van Ginneken
by James Cherry.

The database separates the implementation from the interface, and as a result,
each class becomes two classes, a public one and a private one. For instance,
`dbInst` has the public API functions, while class `_dbInst` has the private
data fields.

The objects are allocated in dynamically resizable tables, the implementation
of which is in `dbTable.hpp`. Each table consists of a number of pages,
each containing 128 objects. The table contains the body of the `struct`,
not a set of pointers. This eliminates most of the pointer overhead while
iteration is accomplished by stepping through the table. Thus, grouping these
objects does not require a doubly-linked list and saves 16 bytes per object
(at the cost of some table overhead). Each object has an id, which is the
index into the table. The lowest 7 bits are the index in the page, while
the higher bits are the page number. Object id's are persistent when saving
and reading the data model to disk, even as pointer addresses may change.

Everything in the data model can be stored on disk and restored from disk
exactly the way it was. An extensive set of equality tests and diff functions
make it possible to check for even the smallest deviation. The capability
to save an exact copy of the state of the system makes it possible to create
a checkpoint. This is a necessary capability for debugging complex systems.

The code follows the definition of LEF and DEF closely and reflects many of
the idiosyncrasies of LEF and DEF. The code defines many types of objects
to reflect LEF and DEF constructs although it sometimes uses different
terminology, for instance, the object to represent a library cell is called
`dbMaster` while the LEF keyword is MACRO.

The data model supports the EEQ and LEQ keywords (i.e., electrically equivalent
and logically equivalent Masters), which could be useful for sizing. However,
it does not support any logic function representation. In general, there is
very limited support for synthesis-specific information: no way to represent
busses, no way to represent logic function, very limited understanding of
signal flow, limited support of timing information, and no support for high
level synthesis or test insertion.

The db represents routing as in DEF, representing a trace from point to point
with a given width. The layout for a net is stored in a class named `dbWire`
and it requires a special `dbWireDecoder` (which works like an iterator)
to unpack the data and another `dbWireEncoder` to pack it. The data model
does not support a region query and objects that are in the same layer are
scattered about the data model and are of different classes.

This means that whatever tool is using the layout information will have to
build its own data structures that are suitable to the layout operations
of that tool. For instance, the router, the extractor, and the DRC engine
would each have to build their unique data structures. This encourages
batch mode operation (route the whole chip, extract the whole chip, run
DRC on the whole chip).

## Limitations

## FAQs

Check out
[GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+odb+in%3Atitle)
about this tool.


## LICENSE

BSD 3-Clause License. See [LICENSE](LICENSE) file.
