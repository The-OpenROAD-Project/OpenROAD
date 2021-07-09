## add_global_connection

### Synopsis
```
  % add_global_connection \
    -net net_name \
    [-inst_pattern inst_regular_expression] \
    -pin_pattern pin_regular_expression
```

### Description
This command is used to connect power and ground pins on design instances to the appropriate supplies

### Options

| Switch Name | Description |
| ----- | ----- |
| -net | Specifies the name of the net in the design to which connections are to be added |
| -inst_pattern | Optional specifies a regular expression to select a set of instances from the design. (Default: .\*) |
| -pin_pattern | Species a regular expression to select pins on the selected instances to connect to the specified net |
| -power | Specifies that the net it a power net |
| -ground | Specifies that the net is a ground net |


### Examples
```
# Stdcell power/ground pins
add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

# RAM power ground pins
add_global_connection -net VDD -pin_pattern {^VDDPE$}
add_global_connection -net VDD -pin_pattern {^VDDCE$}
add_global_connection -net VSS -pin_pattern {^VSSE$}

```

