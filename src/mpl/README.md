# Macro Placement

ParquetFP based macro cell placer. Run `global_placement` before macro placement.
The macro placer places macros/blocks honoring halos, channels and cell row "snapping".

Approximately ceil((#macros/3)^(3/2)) sets corresponding to
quadrisections of the initial placed mixed-size layout are explored and
packed using ParquetFP-based annealing. The best resulting floorplan
according to a heuristic evaluation function kept.

```
macro_placement [-halo {halo_x halo_y}]
                [-channel {channel_x channel_y}]
                [-fence_region {lx ly ux uy}]
                [-snap_layer snap_layer_number]
```

-halo horizontal/vertical halo around macros (microns)
-channel horizontal/vertical channel width between macros (microns)
-fence_region - restrict macro placements to a region (microns). Defaults to the core area.
-snap_layer_number - snap macro origins to this routing layer track

Macros will be placed with max(halo * 2, channel) spacing between macros and the
fence/die boundary. If not solutions are found, try reducing the channel/halo.
