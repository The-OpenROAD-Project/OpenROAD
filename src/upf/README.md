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
    [-file file]
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-file` | `.upf` filename |

### Create Power Domain

```tcl
create_power_domain
    [-elements elements]
    name 
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-elements` | list of module paths that belong this this domain OR `*` for top domain |
| `name` | domain name |

### Create Logic Port

```tcl
create_logic_port
    [-direction direction]
    port_name
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-direction` | direction of the port (`in`, `out`, `inout`) |
| `port_name` | port name |

### Create Power Switch

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
| `-domain` | power domain name |
| `-output_supply_port` | output supply port of the switch |
| `-input_supply_port` | input supply port of the switch |
| `-control_port` | a control port on the switch |
| `-on_state` | {`state_name` `input_supply_port` {`boolean_expression`}} |
| `name` | power switch name |

### Create or Update Isolation Strategy

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
| `-domain` | power domain |
| `-applies_to` | restricts the strategy to apply one of these (`inputs`, `outputs`, `both`) |
| `clamp_value` | value the isolation can drive (`0`, `1`) |
| `isolation_signal` | The control signal for this strategy |
| `isolation_sense` | The active level of isolation control signal |
| `location` | domain in which isolation cells are placed (`parent`, `self`, `fanout`) |
| `update` | flag if use existing strategy, will error if strategy doesn't exist |
| `name` | isolation strategy name |

### Set Interface cell

```tcl 
use_interface_cell
    [-domain domain]
    [-strategy strategy]
    [-lib_cells lib_cells]
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-domain` | power domain name |
| `-strategy` | isolation strategy name |
| `-lib_cells` | list of lib cells that could be used |

### Set Domain Area

```tcl
set_domain_area
    {domain_name -area {llx lly urx ury}}
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `domain_name` | power domain name |
| `llx, lly, urx, ury` | the lower left and upper right x-/y- coordinates respectively of the power domain area (microns) |

## Example scripts

Example script demonstrating how to run UPF related commands can be found here:

```tcl
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
