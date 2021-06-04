## add_pad_libcell
### Synopsis
```
  % add_pad_libcell \
        [-name name] \
        [-type cell_type|-fill|-corner|-bump|-bondpad] \
        [-cell_name cell_names_per_side] \
        [-orient orientation_per_side] \
        [-pad_pin_name pad_pin_name] \
        [-break_signals signal_list] \
        [-physical_only]
```
### Description

In order to create a padring, additional information about the padcells needs to be provided in order to be able to proceed. The add_pad_libcell command is used to specify this additional information.

When specifying the orientation of padcells on a per side basis, use keys bottom, right, top and left. In the case of specifying corner cells use keys ll, lr, ur and ul instead.

One or more libcells can be defined as type filli, when filling a gap between padcells, the largest fill cell that is has a width less than or equal to the gap width will be added, if the gap is too small to fit any fill cell, then the smallest fill cell will be added anyway, on the assumption that the library is designed such that the smallest fill cell is allowed to overlap with padcell instances. At least one libcell of type corner must be defined. For a wirebnd deisgn, if the library uses separate bondpad instance from the padcells, then a bondpad type must be defined. If the design is flipchip, then one libcell definition must be of type bump.

### Options

| Option | Description |
| --- | --- |
| -name | A name to use for the specification of the libcell. This does not need to match the name of the actual cell name in the library |
| -type | Define a type for this cell, which can be used as a reference in the add_pad command |
| -fill | Synonymous with -type fill |
| -corner | Synonymous with -type corner |
| -bondpad | Synonymous with -type bondpad |
| -bump | Synonymous with -type bump |
| -cell_name | Specifies the name of the cell in the library to be used for this padcell. This can either be a single cell name, in which case this cell will always be used, or else it can be a set of key value pairs, with the key being the side of the die on which the pad is to be placed, and the value is the name of the cell to be used on that side. This allows different cells to be used depending upon which edge of the die the cell is to be placed on |
| -orient | Specifies the orientation of the padcell for each of the edges of the die. This is specified as a list of key value pairs, with the key being the side of the die, and the value being the orientation to use for that side |
| -pad_pin_name | Specify the name of the pin on the padcell that is used to connect to the top level pin of the design |
| -breaks | For cells which break the signals which connect by abutment through the padring, specifies the names of the signals that are affected. This is specified as a list of key value pairs, where the key is the name of the signal being broken, and the value is a list of the pins on the left and right hand side of the breaker cell that break this signal. Specify this as an empty list if the breaker cell does not have pins for broken signals |
| -physical_only | Defines the cell to be physical only |

### Examples
```
add_pad_libcell \
  -name PADCELL_SIG \
  -type sig \
  -cell_name {top PADCELL_SIG_V bottom PADCELL_SIG_V left PADCELL_SIG_H right PADCELL_SIG_H} \
  -orient {bottom R0 right R90 top R180 left R270} \
  -pad_pin_name PAD

add_pad_libcell \
  -name PADCELL_VDD \
  -type vdd \
  -cell_name {top PADCELL_VDD_V bottom PADCELL_VDD_V left PADCELL_VDD_H right PADCELL_VDD_H} \
  -orient {bottom R0 right R90 top R180 left R270} \
  -pad_pin_name VDD

add_pad_libcell \
  -name PADCELL_CBRK \
  -type cbk \
  -cell_name {bottom PADCELL_CBRK_V right PADCELL_CBRK_H top PADCELL_CBRK_V left PADCELL_CBRK_H} \
  -orient {bottom R0 right R90 top R180 left R270} \
  -break_signals {RETN {RETNA RETNB} SNS {SNSA SNSB}} \
  -physical_only 1

add_pad_libcell \
  -name PAD_CORNER \
  -corner \
  -orient {ll R0 lr R90 ur R180 ul R270} \
  -physical_only 1
```
