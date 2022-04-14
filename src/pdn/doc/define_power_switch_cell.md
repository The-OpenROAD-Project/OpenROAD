# define_power_switch_cell

## Synopsis
For specifying a power switch cell that will be inserted into a power grid 
```
  % define_power_switch_cell \
    -name <name> \
    -control <control_pin_name> \
    [-acknowledge <acknowledge_pin_name>] \
    -switched_power <switched_power_pin> \
    -power <unswitched_power_pin> \
    -ground <ground_pin> 
```

## Description

Defines the power switches available in the library and specifies the names of the related pins

## Options

| Switch Name | Description |
| ----- | ----- |
| `-name` | The name of the power switch cell. |
| `-control` | The name of the power control port of the power switch cell. |
| `-acknowledge` | Defines the name of the output control signal of the power control switch if it has one. |
| `-switched_power | Defines the name of the pin that outputs the switched power net |
| `-power` | Defines the name of the pin that connects to the unswitched power net. |
| `-ground` | Defines the name of the pin that connects to the ground net. |

## Examples
```
define_power_switch_cell -name POWER_SWITCH -control SLEEP -switched_power VDD -power VDDG -ground VSS

```
