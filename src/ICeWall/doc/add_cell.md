## ICeWall add_cell
### Synopsys
```
  % ICeWall add_cell [-name <name>] \
                     [-type <type>] \
                     [-cell <library_cell>] \
                     [-location {(centre|origin) {x <value> y <value>}}] \
                     [-inst_name <instance_name>]
```
### Description
Use the add_cell command to place an instance at a specific location within the design. This can be used to pre-place marker cells, ESD cells, etc which have a known, fixed location in the design and shold not be moved by the automatic macro placer.

### Examples
```
ICeWall add_cell 
```
## ICeWall add_pad

### Synopsis
```
  % ICeWall add_pad [-name <name>] \
                    [-type <type>] \
                    [-cell <library_cell>] \
                    [-signal <signal_name>] \
                    [-edge (bottom|righttop|left)] \
                    [-location {(centre|origin) {x <value> y <value>}}] \
                    [-bump {row <number> col <number>}] \
                    [-bondpad {(centre|origin) {x <value> y <value>}}] \
                    [-inst_name <instance_name>]
```
### Description

Use the add_pad command to add a padcell to the footprint definition. Use this command before calling the init_footprint command. Use this command as a replacement for the padcell strategy file, or to supplement definitions in an existing padcell strategy file.

The -name switch is optional, but should be used when defining signal names in a separate signal map file.

One of the -cell or -type options is required, which will directly or indirectly associate one of the cells in the library definition with this padcell.

The -signal option can be used to associate the signal name of a top level port with a padcell.

Use the -inst_name option to associate the padcell with a particular instance in the design. e.g. When there are several power/ground padcells in the design, using the -inst_name option will associate a particular instance in the netlist to the specified location on the pad ring. Pad cells with no logical equivalent, e.g. ring breaker cells, an instance will be created of the specified name.

### Options


| Switch_Name | Description |
|--------|-------------|
| -name  | Specify the name of the padcell whne using a separate signal assignment file. A name is automatically generated of not specified |
| -type  | The type of cell specified in the library data |
| -cell  |
| -signal | The name of the signal in the design that is connected to the bondpad/bump of the padcell. Except for nets which have been defined as power or ground nets, only padcell can be associated with a particular signal, and this signal must be a pin at the top level of the design. |
| -edge | Specify which edge of the design the padcell is to be placed on. Valid choices are bottom, rioght, top or left |
| -location | Specify the location of the centre or origin of the padcell |
| -bump | For flipchip designs, declare that the padcell is associated with the bump at the specified row/col location on the die |
| -bondpad | For wirebond designs where the pad cells have separate bondpad instances, use this option to specify the location of the associated bondpad |
| -inst_name | Specify the name of the padcell instance in the design |

### Examples
```
ICeWall add_pad -edge bottom                    -signal p_ddr_dm_2_o -type sig   -location {centre {x 2742.000 y  105.000}} -bondpad {centre {x 2742.000 y   63.293}}
ICeWall add_pad -edge bottom -inst_name u_vss_0 -signal VSS          -type vss   -location {centre {x  397.000 y  105.000}} -bondpad {centre {x  397.000 y  149.893}}
ICeWall add_pad -edge top    -inst_name u_brk0                       -type fbk   -location {centre {x 1587.500 y 2895.000}}

```

