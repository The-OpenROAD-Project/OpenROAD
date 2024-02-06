# Macro Placement

The macro placement module in OpenROAD (`mpl`) is based on 
TritonMacroPlacer, an open-source ParquetFP-based macro cell placer.
The macro placer places macros/blocks honoring halos, channels
and cell row "snapping".
Run `global_placement` before macro placement.

Approximately $\Bigl\lceil [{\frac{numMacros}{3}}]^{1.5} \Bigr\rceil$ quadrisections
of the initial placed mixed-size layout are explored and packed using
ParquetFP-based annealing. The best resulting floorplan according to a
heuristic evaluation function is kept.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Macro Placement

This command performs macro placement.
For placement style, `corner_max_wl` means that choosing the partitions that maximise the wirelength 
of connections between the macros to force them to the corners. Vice versa for `corner_min_wl`.

Macros will be placed with $max(halo * 2, channel)$ spacing between macros, and between
macros and the fence/die boundary. If no solutions are found, try reducing the
channel/halo.

```tcl
macro_placement 
    [-halo {halo_x halo_y}]
    [-channel {channel_x channel_y}]
    [-fence_region {lx ly ux uy}]
    [-snap_layer snap_layer_number]
    [-style corner_wax_wl|corner_min_wl]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-halo` | Horizontal and vertical halo around macros (microns). |
| `-channel` | Horizontal and vertical channel width between macros (microns). |
| `-fence_region` | Restrict macro placements to a region (microns). Defaults to the core area. |
| `-snap_layer` | Snap macro origins to this routing layer track. |
| `-style` | Placement style, to choose either `corner_max_wl` or `corner_min_wl`. The default value is `corner_max_wl`. |


## Useful Developer Commands

If you are a developer, you might find these useful. More details can be found in the [source file](./src/MacroPlacer.cpp) or the [swig file](./src/MacroPlacer.i).

| Command Name | Description |
| ----- | ----- |
| `macro_placement_debug` | Macro placement debugging. Note that GUI must be present for this command, otherwise a segfault will occur. | 

## Example scripts

Example scripts demonstrating how to run TritonMacroPlace on a sample design of `east_west` as follows:

```
./test/east_west.tcl
./test/east_west1.tcl
./test/east_west2.tcl
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following script:

```shell
./test/regression
```

## Limitations

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+mpl) about this tool.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
