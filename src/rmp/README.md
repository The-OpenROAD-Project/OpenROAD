# Restructure

Restructure is an interface to ABC for local resynthesis. The package allows
logic restructuring that targets area or timing. It extracts a cloud of logic
using the OpenSTA timing engine, and passes it to ABC through `blif` interface.
Multiple recipes for area or timing are run to obtain multiple structures from ABC;
the most desirable among these is used to improve the netlist.
The ABC output is read back by a `blif` reader which is integrated to OpenDB.
`blif` writer and reader also support constants from and to OpenDB. Reading
back of constants requires insertion of tie cells which should be provided
by the user as per the interface described below.


## Commands

Restructuring can be done in two modes: area or delay.

### Area Mode

```
restructure -liberty_file <liberty_file>
            -target "area"
            -tielo_pin  <tielo_pin_name>
            -tiehi_pin  <tiehi_pin_name>
```

### Timing Mode

```
restructure -liberty_file <liberty_file>
            -target "delay"
            -slack_threshold <slack_val>
            -depth_threshold <depth_threshold>
            -tielo_pin  <tielo_pin_name>
            -tiehi_pin  <tiehi_pin_name>
```

Argument Description:

-   `liberty_file` Liberty file with description of cells used in design. This
    is passed to ABC.
-   `target` could be area or delay. In area mode, the focus is area reduction
    and timing may degrade. In delay mode, delay is likely reduced but area
    may increase.
-   `-slack_threshold` specifies a (setup) timing slack value below which timing paths need
    to be analyzed for restructuring.
-   `-depth_threshold` specifies the path depth above which a timing path
    would be considered for restructuring.
-   `tielo_pin` specifies the tie cell pin which can drive constant zero. Format
    is lib/cell/pin
-   `tiehi_pin` specifies the tie cell pin which can drive constant one. Format
    is lib/cell/pin

## Example scripts

## Regression tests

## Limitations

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+restructure+in%3Atitle)
about this tool.

## Authors

-   Sanjiv Mathur
-   Ahmad El Rouby

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
