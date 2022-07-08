# add_pad

## Synopsis
```
  % add_pad [-name <name>] \
                    [-type <type>] \
                    [-cell <library_cell>] \
                    [-signal <signal_name>] \
                    [-edge (bottom|right|top|left)] \
                    [-location {(center|origin) {x <value> y <value>} [orient (R0|R90|R180|R270|MX|MY|MXR90|MYR90)]}] \
                    [-bump {row <number> col <number>}] \
                    [-bondpad {(center|origin) {x <value> y <value>}}] \
                    [-inst_name <instance_name>] \
                    [-padcell_to_rdl <list_of_vias>] \
                    [-rdl_to_bump <list_of_vias>] 
```
## Description

Use the add_pad command to add a padcell to the footprint definition. Use this command before calling the init_footprint command. Use this command as a replacement for the padcell strategy file, or to supplement definitions in an existing padcell strategy file.

The -name switch is optional, but should be used when defining signal names in a separate signal map file.

One of the -cell or -type options is required, which will directly or indirectly associate one of the cells in the library definition with this padcell.

The -signal option can be used to associate the signal name of a top-level port with a padcell.

Use the -inst_name option to associate the padcell with a particular instance in the design. For example, when there are several power/ground padcells in the design, using the -inst_name option will associate a particular instance in the netlist to the specified location on the padring. For padcells with no logical equivalent, e.g., ring breaker cells, an instance will be created of the specified name.

The -edge option is used to determine the orientation of the padcell, where the actual orientation of a padcell on a given edge is specified in the library definitions file. The coordinates specified for the location of the padcell are cross-checked against the orientation determined from the -edge option. Similarly, if orient is defined by the -location option, this too will be cross-checked against the value of the edge option. If the -edge option is not defined then the -location setting is used to determine the side on which the padcell appears, hence determining the orientation of the cell. Once again, the orient setting merely serves to act as a cross-check.

If the padcell pad pin is not in the rdl layer, use the -padcell_to_rdl option to specify a list of vias to be placed at the center of the padcell pad pin to connect to the rdl layer. Similarly, the -rdl_to_bump option can be used to specify a list of vias to connect the rdl routing to the bump cell if necessary.

## Options


| Switch_Name | Description |
| ------ | ----------- |
| -name  | Specify the name of the padcell when using a separate signal assignment file. A name is automatically generated if not specified. |
| -type  | The type of cell specified in the library data. |
| -cell  |
| -signal | The name of the signal in the design that is connected to the bondpad/bump of the padcell. Except for nets which have been defined as power or ground nets, only padcell can be associated with a particular signal, and this signal must be a pin at the top level of the design. |
| -edge | Specify which edge of the design the padcell is to be placed on. Valid choices are bottom, right, top or left. |
| -location | Specify the location of the center or origin of the padcell. |
| -bump | For flipchip designs, declare that the padcell is associated with the bump at the specified row/col location on the die. |
| -bondpad | For wirebond designs where the padcells have separate bondpad instances, use this option to specify the location of the associated bondpad. |
| -inst_name | Specify the name of the padcell instance in the design. This takes precedence over the -pad_inst_pattern method described in the set_padring_options command. |
| -padcell_to_rdl | Specify a list of vias to connect the padcell pad pin to the rdl layer if required |
| -rdl_to_bump | Specify a list of vias to connect the rdl to the bump cell if required |

## Examples
```
add_pad -edge bottom                    -signal p_ddr_dm_2_o -type sig   -location {center {x 2742.000 y  105.000}} -bondpad {center {x 2742.000 y   63.293}}

add_pad -edge bottom -inst_name u_vss_0 -signal VSS          -type vss   -location {center {x  397.000 y  105.000}} -bondpad {center {x  397.000 y  149.893}}

add_pad -edge top    -inst_name u_brk0                       -type fbk   -location {center {x 1587.500 y 2895.000}}

```
