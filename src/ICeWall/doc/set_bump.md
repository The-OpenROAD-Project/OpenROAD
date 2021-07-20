# set_bump_options

## Synopsis
```
  % set_bump \
        -row row \
        -col col \
        [(-power|-ground|-net) net_name] \
        [-remove] \
```

## Description
The set_bump command is used to provide additional information about specific bumps in the bump array.

The -row and -col options are required and identify the row and column of a specific bump location in the bump array. The bump in the top left corner of the array is row 1, column 1.

The -net, -power and -ground options are mutually exclusive and are used to specify the name of the net connected to a bump and whether it is of type signal, power or ground.

The -remove option is used to specify that no bump should be placed at the specified position in the bump array

## Options

| Option | Description |
| --- | --- |
| -row | Specifies the row of a specific bump. |
| -col | Specifies the column of a specific bump. |
| -net | Specifies the name of the signal net connected to the specified bump. |
| -power | Specifies the name of the power net connected to the specified bump. |
| -ground | Specifies the name of the ground net connected to the specified bump. |
| -remove | Removes the specified bump from the bump array. |

## Examples
```
set_bump -row  8 -col  8 -power VDD1
set_bump -row  9 -col  8 -remove
set_bump -row 10 -col  8 -power VDD2
```

