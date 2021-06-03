## place_cell
### Synopsis
```
  % place_cell [-name <name>] \
                     [-type <type>] \
                     [-cell <library_cell>] \
                     -location {(centre|origin) {x <value> y <value>} orient (R0|R90|R180|R270|MX|MY|MXR90|MYR90)} \
                     [-inst_name <instance_name>]
```
### Description
Use the place_cell command to place an instance at a specific location within the design. This can be used to pre-place marker cells, ESD cells, etc which have a known, fixed location in the design and should not be moved by the automatic macro placer.

One of the -cell or -type options is required, which will directly or indirectly associate one of the cells in the library definition with this padcell.

### Options


| Switch_Name | Description |
| ------ | ----------- |
| -name  | Specify the name of the padcell when using a separate signal assignment file. A name is automatically generated if not specified |
| -type  | The type of cell specified in the library data |
| -cell  | Specify the name of the cell in the library to be added as an instance in the design |
| -location | Specify the location of the centre or origin along with the orientation of the cell |
| -inst_name | Specify the name of the padcell instance in the design |

### Example
```
place_cell -name marker0 -type marker -inst_name u_marker_0 -location {centre {x 1200.000 y 1200.000} orient R0}
```
