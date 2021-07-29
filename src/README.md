# OpenROAD

OpenROAD is run using Tcl scripts. The following commands are used to read
and write design data.

``` shell
read_lef [-tech] [-library] filename
read_def filename
write_def [-version 5.8|5.6|5.5|5.4|5.3] filename
read_verilog filename
write_verilog filename
read_db filename
write_db filename
write_abstract_lef filename
```

Use the Tcl `source` command to read commands from a file.

``` shell
source [-echo] file
```

If an error is encountered in a command while reading the command file,
the error is printed and no more commands are read from the file. If
`file_continue_on_error` is `1` OpenROAD will continue reading commands
after the error.

If `exit_on_error` is `1` OpenROAD will exit when it encounters an
error.

OpenROAD can be used to make a OpenDB database from LEF/DEF, or
Verilog (flat or hierarchical). Once the database is made it can be
saved as a file with the `write_db` command. OpenROAD can then read
the database with the `read_db` command without reading LEF/DEF or
Verilog.

The `read_lef` and `read_def` commands can be used to build an OpenDB
database as shown below. The `read_lef -tech` flag reads the
technology portion of a LEF file.  The `read_lef -library` flag reads
the MACROs in the LEF file.  If neither of the `-tech` and `-library`
flags are specified they default to `-tech -library` if no technology
has been read and `-library` if a technology exists in the database.

``` shell
read_lef liberty1.lef
read_def reg1.def
# Write the db for future runs.
write_db reg1.db
```

The `read_verilog` command is used to build an OpenDB database as
shown below. Multiple verilog files for a hierarchical design can be
read.  The `link_design` command is used to flatten the design
and make a database.

``` shell
read_lef liberty1.lef
read_verilog reg1.v
link_design top
# Write the db for future runs.
write_db reg1.db
```

## Example scripts

Example scripts demonstrating how to run OpenROAD on sample designs
can be found in /test. Flow tests taking sample designs from synthesis
verilog to routed design in the open source technologies Nangate45 and
Sky130 are shown below.

``` shell
gcd_nangate45.tcl
aes_nangate45.tcl
tinyRocket_nangate45.tcl
gcd_sky130.tcl
aes_sky130.tcl
ibex_sky130.tcl
```

Each of these designs use the common script `flow.tcl`.

## Abstract Lef Support
OpenROAD contains an abstract lef writer that can take your current design
and emit an abstract lef representing the external pins of your design and metal
obstructions.


``` tcl
read reg1.db
write_abstract_lef reg1_abstract.lef
```

### Limitations of the Abstract Lef Writer
Currently the writer will place an obstruction over the entire block area on any
metal layer if there is any object on that metal layer.

## TCL functions

Get the die and core areas as a list in microns: "llx lly urx ury"

```
ord::get_die_area
ord::get_core_area
```
