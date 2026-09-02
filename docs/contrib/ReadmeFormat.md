# Tool Name

The top-level READMEs in each tool folder (`~/OpenROAD/src/<tool>/README.md`)
has to be formatted in this particular manner. For most part, you can copy 
the format and replace the contents where necessary.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Command ABC

```{note}
Please add a description, even a one-liner will be sufficient to 
avoid triggering CI errors.
```

The `command_abc` command performs...

The developer arguments are...

Note for commands, you must adhere to the top-level Tcl definition
for the same command. 
- Arguments: cannot be preceded with dashes 
- Flags/Keys: verify if it is optional or required, then insert the 
necessary square brackets. Also, keys have to be followed with a specifier
whereas flags only require the `-flag` itself.

Place the positional arguments last.

```tcl
command_abc
    -key1 key1   
    [-flag1]
    [-flagDev]
    [-key2 key2]
    arg1
```

Arguments must follow this order and be sorted alphabetically within each category:

1. Mandatory flags
2. Optional flags
3. Positional

#### Options

Every row must have a non-empty description. Where the switch takes a value,
state its **type**, the accepted **range** when one applies, and — for optional
switches — the **default** used when the switch is omitted. Readers cannot
infer any of this from the synopsis, and a missing default is the most common
gap in the existing tool READMEs.

| Switch Name | Description |
| ----- | ----- |
| `arg1` | Description for `arg1`. The type is `string`. |
| `-key1` | Description for `key1`. The type is `int`. Required. |
| `-flag1` | Description for `flag1`. Flags take no value, so they state no type or default. |
| `-key2` | Description for `key2`. The type is `int` in the range `[25-50]`. The default is `42`. |

#### Developer Arguments

If there are some developer arguments you want to highlight to the end user
not to worry about - you can park them in the same level below the main
`Options` category. 

| Switch Name | Description |
| ----- | ----- |
| `-flagDev` | Description for `flagDev`. |

## Useful Developer Commands

If you are a developer, you might find these useful. Link to the module's
source file and swig file using paths relative to this README, substituting
the real file names for `ToolName`:

```markdown
More details can be found in the [source file](./src/ToolName.cpp) or the
[swig file](./src/ToolName.i).
```

Do not leave the link targets empty — verify each path exists before you
commit.

| Command Name | Description |
| ----- | ----- |
| `command_abc_debug` | Debug something. |

## Example scripts

Examples scripts demonstrating ... 

```shell
./test/asdfg.tcl
```

## Regression tests

There are a set of regression tests in `./test`. Refer to this [section](../../README.md#regression-tests) for more information.

Simply run the following script: 

```shell
./test/regression
```

## Limitations

## FAQs

Check out the [GitHub Q&A discussions](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a)
about this tool.

## Authors

## References

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
