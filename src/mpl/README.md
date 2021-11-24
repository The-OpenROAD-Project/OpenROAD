# Macro Placement

ParquetFP-based macro cell placer, "TritonMacroPlacer".
The macro placer places macros/blocks honoring halos, channels
and cell row "snapping".
Run `global_placement` before macro placement.

Approximately ceil((#macros/3)^(3/2)) sets corresponding to quadrisections
of the initial placed mixed-size layout are explored and packed using
ParquetFP-based annealing. The best resulting floorplan according to a
heuristic evaluation function is kept.

## Commands

```
macro_placement [-halo {halo_x halo_y}]
                [-channel {channel_x channel_y}]
                [-fence_region {lx ly ux uy}]
                [-snap_layer snap_layer_number]
```

-   `-halo` horizontal/vertical halo around macros (microns)
-   `-channel` horizontal/vertical channel width between macros (microns)
-   `-fence_region` - restrict macro placements to a region (microns). Defaults to the core area.
-   `-snap_layer_number` - snap macro origins to this routing layer track

Macros will be placed with `max(halo * 2, channel)` spacing between macros, and between
macros and the fence/die boundary. If no solutions are found, try reducing the
channel/halo.

## Example scripts

## Regression tests

## Limitations

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+macro%20place+in%3Atitle)
about this tool.

## License

BSD 3-Clause License.
