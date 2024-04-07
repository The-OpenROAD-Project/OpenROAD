# Utilities

The utility module contains the `man` command.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Man

This can be used for a range of commands in different levels as follows:
- Level 1: Top-level openroad command (e.g. `man openroad`)
- Level 2: Individual module commands (e.g. `man clock_tree_synthesis`)
- Level 3: Info, error, warning messages (e.g. `man CTS-0001`)

```tcl
man
    name
    [-manpath manpath]
    [-no_query]
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-name` | Name of the command/message to query. |
| `-manpath` | Include optional path to manpage pages (e.g. ~/OpenROAD/docs/cat). |
| `-no_query` | This flag determines whether you wish to see all of the man output at once. Default value is `False`, which shows a buffered output. |

## Example scripts

You may run various commands or message IDs for manpages.
```
man openroad
man clock_tree_synthesis
man CTS-0005
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests).

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
