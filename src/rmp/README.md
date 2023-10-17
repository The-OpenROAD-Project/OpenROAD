# Restructure

The restructure module in OpenROAD (`rmp`) is based on 
an interface to ABC for local resynthesis. The package allows
logic restructuring that targets area or timing. It extracts a cloud of logic
using the OpenSTA timing engine, and passes it to ABC through `blif` interface.
Multiple recipes for area or timing are run to obtain multiple structures from ABC;
the most desirable among these is used to improve the netlist.
The ABC output is read back by a `blif` reader which is integrated to OpenDB.
`blif` writer and reader also support constants from and to OpenDB. Reading
back of constants requires insertion of tie cells which should be provided
by the user as per the interface described below.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

Restructuring can be done in two modes: area or delay.

### Area Mode

```tcl
restructure 
    -liberty_file liberty_file
    -target area
    -tielo_pin  tielo_pin_name
    -tiehi_pin  tiehi_pin_name
```

### Timing Mode

```tcl
restructure 
    -liberty_file liberty_file
    -target delay
    -slack_threshold slack_val
    -depth_threshold depth_threshold
    -tielo_pin  tielo_pin_name
    -tiehi_pin  tiehi_pin_name
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-liberty_file` | Liberty file with description of cells used in design. This is passed to ABC. |
| `-target` | Either `area` or `delay`. In area mode, the focus is area reduction, and timing may degrade. In delay mode, delay is likely reduced, but the area may increase. |
| `-slack_threshold` | Specifies a (setup) timing slack value below which timing paths need to be analyzed for restructuring. |
| `-depth_threshold` | Specifies the path depth above which a timing path would be considered for restructuring. |
| `-tielo_pin` | Tie cell pin that can drive constant zero. The format is `lib/cell/pin`. |
| `-tiehi_pin` | Tie cell pin that can drive constant one. The format is `lib/cell/pin`. |

## Example scripts

Example scripts on running `rmp` for a sample design of `gcd` as follows:

```tcl
./test/gcd_restructure.tcl
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following script:

```shell
./test/regression
```

## Limitations

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+restructure)
about this tool.

## Authors

-   Sanjiv Mathur
-   Ahmad El Rouby

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
