# place_cell
## Synopsis
```
  % place_cell -inst_name <inst_name> \
                 [-cell <library_cell>] \
                 -origin <point>] \
                 -orient (R0|R90|R180|R270|MX|MY|MXR90|MYR90) \
                 [-status (PLACED|FIRM)]
```
## Description
Use the place_cell command to place an instance at a specific location within the design. This can be used to pre-place marker cells, ESD cells, etc which have a known, fixed location in the design and should not be moved by the automatic macro placer.

One of the -inst_name, -origin and -orient options are required. If there is no instance <inst_name> in the netlist of the design, then the -cell option is required, and an instance of the specified <library_cell> will be added to the design. If an instance <inst_name> does exist in the design and the -cell option is specified, it is an error if the instance in the netlist does not match the specified <library_cell>.

## Options


| Switch_Name | Description |
| ------ | ----------- |
| -inst_name | Specify the name of the padcell instance in the design |
| -name  | Specify the name of the padcell when using a separate signal assignment file. A name is automatically generated if not specified |
| -cell  | Specify the name of the cell in the library to be added as an instance in the design |
| -origin | Specify the origin of the cell as a point, i.e. a list of two numbers |
| -orient | Specify a valid orientation for the placement of the cell |

## Example
```
place_cell -cell MARKER -inst_name u_marker_0 -origin {1197.5 1199.3} -orient R0 -status FIRM
```
