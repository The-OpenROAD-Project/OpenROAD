## set_voltage_domain

### Synopsis
```
  % set_voltage_domain
    -name name \
    -power power_net \
    -ground ground_net \
    [-region region_name]
```

### Description

Defines a named voltage domain with the names of the power and ground nets for a region. 

The -region argument specifies the name of a region of the design. This region must already exist in the floorplan before referencing it with the set_voltage_domain command. If the -region argument is not supplied then region is the entire extent of the design.

A default voltage domain called CORE exists with power net VDD and ground net VSS.

The -name argument is used to define a name for the voltage domain that can be used in the [define_pdn_grid](define_pdn_grid.md) command
The -power and -ground arguments are used to define the names of the nets to be use for power and ground respectively within this voltage domain.

### Options

| Switch Name | Description |
| ----- | ----- |
| -name | Defines a name to use when referring to this grid definition |
| -power | Specifies the name of the power net for this voltage domain |
| -ground | Specifies the name of the ground net for this voltage domain |
| -region | Specifies a region of the design occupied by this voltage domain |

### Examples
```
set_voltage_domain -name CORE -power_net VDD -ground_net VSS
set_voltage_domain -name TEMP_ANALOG -region TEMP_ANALOG -power_net VIN -ground_net VSS
```

