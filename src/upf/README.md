# Read UPF Utility

This module contains functionality to read, and modify information
from Unified Power Format (UPF) files. 

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Read UPF

Sources the UPF file. 

```tcl
read_upf
    -file file
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-file` | Path to `.upf` file. |

### Write UPF

Writes the UPF file.

```tcl
write_upf file_name
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `file_name` | Filename argument. |

### Create Power Domain

This command creates power domain for a group of modules.

```tcl
create_power_domain
    [-elements elements]
    name 
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-elements` | List of module paths that belong this this domain OR `*` for top domain. |
| `name` | Domain name. |

### Create Logic Port

This command creates logic port. Direction must be specified from:
`in`, `out`, `inout`.

```tcl
create_logic_port
    [-direction direction]
    port_name
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-direction` | Direction of the port (`in`, `out`, `inout`). |
| `port_name` | Port name. |

### Create Power Switch

This command creates power switch.

```tcl
create_power_switch
    [-domain domain]
    [-output_supply_port output_supply_port]
    [-input_supply_port input_supply_port]
    [-control_port control_port]
    [-on_state on_state]
    name
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-domain` | Power domain name. |
| `-output_supply_port` | Output supply port of the switch. |
| `-input_supply_port` | Input supply port of the switch. |
| `-control_port` | Control port on the switch. |
| `-on_state` | One of {`state_name`, `input_supply_port`, `boolean_expression`}. |
| `name` | Power switch name. |

### Create or Update Isolation Strategy 

This command creates or update isolation strategy.

```tcl 
set_isolation
    [-domain domain]
    [-applies_to applies_to]
    [-clamp_value clamp_value]
    [-isolation_signal isolation_signal]
    [-isolation_sense isolation_sense]
    [-location location]
    [-update]
    name
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-domain` | Power domain |
| `-applies_to` | Restricts the strategy to apply one of these (`inputs`, `outputs`, `both`). |
| `-clamp_value` | Value the isolation can drive (`0`, `1`). |
| `-isolation_signal` | The control signal for this strategy. |
| `-isolation_sense` | The active level of isolation control signal. |
| `-location` | Domain in which isolation cells are placed (`parent`, `self`, `fanout`). |
| `-update` | Only available if using existing strategy, will error if the strategy doesn't exist. |
| `name` | Isolation strategy name. |

### Set Interface cell

This command sets the interface cell.

```tcl 
use_interface_cell
    [-domain domain]
    [-strategy strategy]
    [-lib_cells lib_cells]
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-domain` | Power domain name. |
| `-strategy` | Isolation strategy name. |
| `-lib_cells` | List of lib cells that could be used. |

### Set Domain Area

This command sets the power domain area.

```tcl
set_domain_area
    domain_name
    -area {llx lly urx ury}
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `domain_name` | Power domain name. |
| `-area` | x-/y- coordinates in microns for the lower left and upper right corners of the power domain area. |


### Map existing power switch

This command maps existing power switch.

```tcl 
map_power_switch
    [-switch_name_list switch_name_list]
    [-lib_cells lib_cells]
    [-port_map port_map]
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-switch_name_list` |  A list of switches (as defined by create_power_switch) to map. |
| `-lib_cells` | A list of library cells that could be mapped to the power switch |
| `-port_map` | A map that associates model ports defined by create_power_switch to logical ports |

### Set Level Shifter

This command sets level shifter.
Options coming soon.

```tcl
set_level_shifter
    [-domain domain] \
    [-elements elements] \
    [-exclude_elements exclude_elements] \
    [-source source] \
    [-sink sink] \
    [-use_functional_equivalence use_functional_equivalence] \
    [-applies_to applies_to] \
    [-applies_to_boundary applies_to_boundary] \
    [-rule rule] \
    [-threshold threshold] \
    [-no_shift] \
    [-force_shift] \
    [-location location] \
    [-input_supply input_supply] \
    [-output_supply output_supply] \
    [-internal_supply internal_supply] \
    [-name_prefix name_prefix] \
    [-name_suffix name_suffix] \
    [-instance instance] \
    [-update] \
    [-use_equivalence use_equivalence] \
    name 
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-domain` | |
| `-elements` | |
| `-exclude_elements` | |
| `-source` | |
| `-sink` | |
| `-use_functional_equivalence` | |
| `-applies_to` | |
| `-applies_to_boundary` | |
| `-rule` | |
| `-threshold` | |
| `-no_shift` | |
| `-force_shift` | |
| `-location` | |
| `-input_supply` | |
| `-output_supply` | |
| `-internal_supply` | |
| `-name_prefix` | |
| `-name_suffix` | |
| `-instance` | |
| `-update` | |
| `-use_equivalence` | |
| `name` | |

### Set Domain Voltage

This command sets the voltage of a power domain.

```tcl
set_domain_voltage
    [-domain domain] \
    [-voltage voltage] 
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-domain` | Power domain name. |
| `-voltage` | Power domain voltage. The allowed values are `float`, default value is `0.0`. |

### Set Level Shifter Cell

This command sets the library cell used for level shifter.

```tcl
set_level_shifter_cell
    [-level_shifter level_shifter] \
    [-cell_name cell_name] \
    [-input_port input_port] \
    [-output_port output_port] 
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-level_shifter` | KIV. |
| `-cell_name` | KIV. |
| `-input_port` | KIV. |
| `-output_port` | KIV. |

## Example scripts

Example script demonstrating how to run `upf` related commands can be found here:

```
./test/upf_test.tcl
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests). 

Simply run the following script: 

```shell
./test/regression
```

## Limitations

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+upf)
about this tool.

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
