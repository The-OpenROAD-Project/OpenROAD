## define_pdn_grid

### Synopsis
For specifying a power grid over the stdcell area
```
  % define_pdn_grid
    [-name <name>] \
    [-pins <list_of_pin_layers>] \
    [-starts_with (POWER|GROUND)] \
    [-voltage_domain <list_of_domain_names>] \
    [-starts_with (POWER|GROUND)]
```
For specifying a power grid over macros in the design
```
  % define_pdn_grid
    -macro \
    [-name name] \
    [-orient <list_of_valid_orientations>] \
    [-instances <list_of_instances] \
    [-cells <list_of_cells>] \
    [-pin_direction (horizontalvertical)] \
    [-halo <list_of_halo_values>] \
    [-voltage_domain <list_of_domain_names>] \
    [-starts_with (POWER|GROUND)]    
```

### Description

Defines the rules to describe a power grid pattern to be placed in the design.

The design is made up of one or more voltage domain regions, specified using the [set_voltage_domain](set_voltage_domain.md) command. 

The -voltage_domain argument is used to specify the voltage domain(s) to which this grid is to be applied. If no voltage domain is specified then the default domain CORE is assumed.

Rules for adding stdcell rails and supply straps can be added to the grid specificatoin using the [add_pdn_stripe](add_pdn_stripe.md) command.
Rules for adding rings around the grid can be added to the grid specification using the [add_pdn_ring](add_pdn_ring.md) command.
Connections between layers are specified using the [add_pdn_connect](add_pdn_connect.md) command.

The -name argument is used to create a name for the grid that can be used in subsequent add_pdn_* commands. If the -name argument is not provided, then a name will be created in the form <voltage_domain1>(_<voltage_domainN>)_(stdcell|macro)_<idx>

The -pins argument is used to create power and ground pins on the power and ground stripes of the specified layers.

The -starts_with argument is used to define whether the power net, or ground net is added as the first in a power/ground pair (Default: GROUND)

The presence of macros in the design interupts the normal power grid pattern, and additional power grids are defined over the macros in order to control the grid over the macro and which layers in the normal grid pattern are blocked around the macro.

The -macro flag is used to declare that this grid definition is for macros in the design. All macro cell instances will have this grid, but this list of instances can be filtered using the -instances, -cells and/or -orient arguments

The -instances argument filters the list of instances for which this grid applies, such that only the specified instances are retained.
The -cells argument filters the list of instance for which this grid applies, such that only instances of the specified cells are retained.
The -orient argument filters the list of instances for which this grid applies, such that only instances of the specified orientation are retained.

The -pin_direction argument is used to specifiy whether the power/ground pins on the selected macro instances are oriented in the horizontal or vertical direction.

A halo value is used to specify the minimum distance between this macro and any adjacent cell. If there is a halo value specified for the macro cell in the LEF, then this value will be used. Otherwise the value specified with the -halo option will be used to define the halo. The value for halo can be 1, 2 or 4 numbers; if 1 number is specified, this will be used for all four sides of the macro; if 2 numbers are specified, the first will be used for the separation on left/right sides and the second number will be used for the top/bottom sides; if 4 numbers are specified then these correspond to halos on left, bottom, right and top sides respectively.

Each of the -instances, -cells and -orient acts as an independent filter applied in succession to the list of macros.


### Options

| Switch Name | Description |
| ----- | ----- |
| -name | Defines a name to use when referring to this grid definition |
| -voltage_domain | Defines the name of the voltage domain for this grid. (Default: CORE) |
| -pins | Defines a list of layers which where the power straps will be promoted to block pins |
| -starts_with | Specifies whether the first strap placed will be POWER or GROUND (Default: GROUND) |
| -macro | Defines the type of grid being added, can be either stdcell or macro|
| -instances | For a macro, defines a set of valid instances. Macros with a matching instance name will use this grid specification |
| -cells | For a macro, defines a set of valid orientations. Macros which are instances of one of these cells will use this grid specification |
| -orient | For a macro, defines a set of valid orientations. Macros with one of the valid orientations will use this grid specification |
| -halo | Specifies the default minimum separation of selected macros from other cells in the design. This is only used if the macro does not define halo values in the LEF description. If 1 value is specified it will be used on all 4 sides, if two values are specified, the first will be applied to left/right sides and the second will be applied to top/bottom sides, if 4 values are specified, then they are applied to left, bottom, right and top sides respectively.(Default: 0) |
| -pin_direction | Specifies the direction of power/ground pins on the selected macro instances as eiher horizontal or vertical |


### Examples
```
define_pdn_grid -name main_grid -pins {metal7} -voltage_domain {CORE TEMP_ANALOG}

define_pdn_grid -macro -name ram          -orient {R0 R180 MX MY}        -starts_with POWER -pin_direction vertical
define_pdn_grid -macro -name rotated_rams -orient {R90 R270 MXR90 MYR90} -starts_with POWER -pin_direction horizontal

```

