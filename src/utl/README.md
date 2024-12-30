# Utilities

The utility module contains the `man` command.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

## Man installation

The `man` command can be installed optionally as part of the OpenROAD
binary. For more instructions, please refer to [here](manREADME.md).

### Man

The `man` command in OpenROAD is similar in functionality to Unix
(and Unix-like operating systems such as Linux) . It is used to 
display the manual pages for various applications, tools and error 
messages. These manual pages provide detailed information about how
to use a particular command or function, along with its syntax and options.

This can be used for a range of commands in different levels as follows:
- Level 1: Top-level openroad command (e.g. `man openroad`)
- Level 2: Individual module commands (e.g. `man clock_tree_synthesis`)
- Level 3: Info, error, warning messages (e.g. `man CTS-0001`)

```tcl
man
    name
    [-manpath manpath]
    [-no_pager]
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `name` | Name of the command/message to query. |
| `-manpath` | Include optional path to man pages (e.g. ~/OpenROAD/docs/cat). |
| `-no_pager` | This flag determines whether you wish to see all of the man output at once. Default value is `False`, which shows a buffered output. |

## Example scripts

You may run various commands or message IDs for man pages.
```
man openroad
man clock_tree_synthesis
man CTS-0005
```

### tee

Redirect a commands output to a file and standard out.

```tcl
tee (-file filename | -variable name)
    [-append]
    [-quiet]
    command
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-file filename` | File to redirect output into. |
| `-variable name` | Direct output into a variable. |
| `-append` | Append to file. |
| `-quiet` | Do not send output to standard out. |
| `command` | Command to execute. |

## Example scripts

```
tee -file output.rpt { report_design_area }
tee -quiet -file output.rpt { report_floating_nets }
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests). 

For information regarding the Man page test framework, refer to this
[link](../../docs/src/test/README.md).

You should also be aware of the [README](../../docs/contrib/ReadmeFormat.md) and [Tcl](../../docs/contrib/TclFormat.md) format enforced to ensure
accurate parsing of man pages. 

Simply run the following script:

```shell
./test/regression
```

## Limitations

## FAQs

Check out
[GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+utl) about this tool.

## References

## Authors

MAN command is written by Jack Luar with guidance from members of the OpenROAD team,
including: Cho Moon, Matt Liberty. 

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
