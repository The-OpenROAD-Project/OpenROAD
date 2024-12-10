# OpenROAD API

OpenROAD can be run using Tcl, and Python (limited support).
The following commands are used to read and write design data.

````{eval-rst}
.. tabs::

   .. code-tab:: tcl

      read_lef [-tech] [-library] filename
      read_def filename
      write_def [-version 5.8|5.7|5.6|5.5|5.4|5.3] filename
      read_verilog filename
      write_verilog filename
      read_db filename
      write_db filename
      write_abstract_lef filename

   .. code-tab:: python
   
      # read_verilog, write_verilog and write_abstract_lef are not supported in Python.
      read_lef(db: odb.dbDatabase, path: str) -> odb.dbLib
      read_def(tech: odb.dbTech, path: str) -> odb.dbChip
      write_def(block: dbBlock, path: str, version: Optional[odb.defout.Version]) -> int
      read_db(db: odb.dbDatabase, db_path: str) -> odb.dbDatabase
      write_db(db: odb.dbDatabase, db_path: str) -> int

````


Use the Tcl `source` command to read commands from a file.

````{eval-rst}
.. tabs::

   .. code-tab:: tcl

      source [-echo] file

   .. code-tab:: py

      # Source is not supported in Python.
      # Instead run this at the start:
      openroad -python script.py
````

If an error is encountered in a command while reading the command file,
then the error is printed and no more commands are read from the file. If
`file_continue_on_error` is `1` then OpenROAD will continue reading commands
after the error.

If `exit_on_error` is `1` then OpenROAD will exit when it encounters an error.

OpenROAD can be used to make a OpenDB database from LEF/DEF, or Verilog
(flat or hierarchical). Once the database is made it can be saved as a file
with the `write_db` command. OpenROAD can then read the database with the
`read_db` command without reading LEF/DEF or Verilog.

The `read_lef` and `read_def` commands can be used to build an OpenDB database
as shown below. The `read_lef -tech` flag reads the technology portion of a
LEF file.  The `read_lef -library` flag reads the MACROs in the LEF file.
If neither of the `-tech` and `-library` flags are specified they default
to `-tech -library` if no technology has been read and `-library` if a
technology exists in the database.

````{eval-rst}
.. tabs::

   .. code-tab:: tcl

      read_lef liberty1.lef
      read_def reg1.def
      # Write the db for future runs.
      write_db reg1.db


   .. code-tab:: py

      from openroad import Design, Tech
      tech = Tech()
      tech.readLef("liberty1.lef")
      design = Design(tech)
      design.readDef("reg1.def")

      # Write the db for future runs.
      design.writedb("reg1.db")
````

The `read_verilog` command is used to build an OpenDB database as shown
below. Multiple Verilog files for a hierarchical design can be read.
The `link_design` command is used to flatten the design and make a database.


````{eval-rst}
.. tabs::

   .. code-tab:: tcl

      read_lef liberty1.lef
      read_verilog reg1.v
      link_design top
      # Write the db for future runs.
      write_db reg1.db


   .. code-tab:: py

      # Not supported in Python
````

## Example scripts

Example scripts demonstrating how to run OpenROAD on sample designs can
be found in `/test`. Flow tests taking sample designs from synthesizable RTL Verilog
to detail-routed final layout in the open-source technologies Nangate45 and Sky130HD are
shown below.

``` shell
gcd_nangate45.tcl
aes_nangate45.tcl
tinyRocket_nangate45.tcl
gcd_sky130hd.tcl
aes_sky130hd.tcl
ibex_sky130hd.tcl
```

Each of these designs use the common script `flow.tcl`.

## Abstract LEF Support

OpenROAD contains an abstract LEF writer that can take your current design
and emit an abstract LEF representing the external pins of your design and
metal obstructions.


``` tcl
write_abstract_lef (-bloat_factor bloat_factor|-bloat_occupied_layers) \
		   filename
```
### Options

| Switch Name | Description |
| ----- | ----- |
| `-bloat_factor` | Specifies the bloat factor used when bloating then merging shapes into LEF obstructions. The factor is measured in # of default metal pitches for the respective layer. A factor of `0` will result in detailed LEF obstructions |
| `-bloat_occupied_layers` | Generates cover obstructions (obstructions over the entire layer) for each layer where shapes are present |

### Examples
```
read reg1.db

# Bloat metal shapes by 3 pitches (respectively for every layer) and then merge
write_abstract_lef -bloat_factor 3 reg1_abstract.lef

# Produce cover obstructions for each layer with shapes present
write_abstract_lef -bloat_occupied_layers reg1_abstract.lef
```

### Global Connections

#### Add global connections

The `add_global_connection` command is used to specify how to connect power and ground pins on design instances to the appropriate supplies.

```
add_global_connection -net net_name \
                      [-inst_pattern inst_regular_expression] \
                      -pin_pattern pin_regular_expression \
                      (-power|-ground) \
                      [-region region_name]
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| `-net` | Specifies the name of the net in the design to which connections are to be added |
| `-inst_pattern` | Optional specifies a regular expression to select a set of instances from the design. (Default: .\*) |
| `-pin_pattern` | Species a regular expression to select pins on the selected instances to connect to the specified net |
| `-power` | Specifies that the net it a power net |
| `-ground` | Specifies that the net is a ground net |
| `-region` | Specifies the name of the region for this rule |

##### Examples
```
# Stdcell power/ground pins
add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

# SRAM power ground pins
add_global_connection -net VDD -pin_pattern {^VDDPE$}
add_global_connection -net VDD -pin_pattern {^VDDCE$}
add_global_connection -net VSS -pin_pattern {^VSSE$}
```

#### Perform global connections

The `global_connect` command is used to connect power and ground pins on design instances to the appropriate supplies.

```
global_connect
```

#### Clear global connection rules

The `clear_global_connect` command is used remove all defined global connection rules.

```
clear_global_connect
```

#### Report global connection rules

The `report_global_connect` command is used print out the currently defined global connection rules.

```
report_global_connect
```

#### Report cell type usage

The `report_cell_usage` command is used to print out the usage of cells for each type of cell.

```
report_cell_usage [-verbose] [module instance]
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| `-verbose` | Add information about all leaf instances. |
| `module instance` | Report cell usage for a specified module instance. |

## TCL functions

Get the die and core areas as a list in microns: `llx lly urx ury`

```
ord::get_die_area
ord::get_core_area
```

The `place_inst` command is used to place an instance.  If -cell is
given then a new instance may be created as well as placed.

```
place_inst -name inst_name \
           (-origin xy_origin | -location xy_location) \
           [-orientation orientation] \
           [-cell library_cell] \
           [-status status]
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| `-name` | The name of the instance |
| `-orientaton` | The orientation of the instance. Default is R0 |
| `-origin` | The x and y coordinates for where the origin of the instance is placed. |
| `-location` | The x and y coordinates for where the instance is placed. |
| `-cell` | Required if a new instance is to be created. |
| `-status` | The placement status of the instance. Default is PLACED |


## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+openroad+in%3Atitle)
about this tool.

## License

BSD 3-Clause License.
