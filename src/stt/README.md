# Rectilinear Steiner Tree 

The steiner tree (`stt`) module in OpenROAD constructs steiner trees used in
the global routing (`grt`) module. Documentation is current under construction.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Set Routing Alpha

This command sets routing alphas for a given net `net_name`.

By default the global router uses heuristic rectilinear Steiner minimum
trees (RSMTs) as an initial basis to construct route guides. An RSMT
tries to minimize the total wirelength needed to connect a given set
of pins.  The Prim-Dijkstra heuristic is an alternative net topology
algorithm that supports a trade-off between total wirelength and maximum
path depth from the net driver to its loads. The `set_routing_alpha`
command enables the Prim/Dijkstra algorithm and sets the alpha parameter
used to trade-off wirelength and path depth.  Alpha is between 0.0
and 1.0. When alpha is 0.0 the net topology minimizes total wirelength
(i.e. capacitance).  When alpha is 1.0 it minimizes longest path between
the driver and loads (i.e., maximum resistance).  Typical values are
0.4-0.8. You can call it multiple times for different nets.

Example: `set_routing_alpha -net clk 0.3` sets the alpha value of 0.3 for net *clk*.

```tcl
set_routing_alpha 
    [-net net_name] 
    [-min_fanout fanout]
    [-min_hpwl hpwl]
    [-clock_nets]
    alpha
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-net` | Net name. | 
| `-min_fanout` | Set the minimum number for fanout. | 
| `-min_hpwl` | Set the minimum half-perimetere wirelength (microns). | 
| `-clock_nets` | If this flag is set to True, only clock nets will be modified. |
| `alpha` | Float between 0 and 1 describing the trade-off between wirelength and path depth. |

## Example scripts

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests). 

Simply run the following script: 

```shell
./test/regression
```

## Limitations

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+stt+in%3Atitle)
about this tool.

## References

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.

