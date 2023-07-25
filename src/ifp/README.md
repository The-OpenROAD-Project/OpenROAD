# Initialize Floorplan

This tool initializes floorplan constraints, die/core area, and makes tracks. 

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Initialize Floorplan

```tcl
initialize_floorplan
  [-utilization util]
  [-aspect_ratio ratio]
  [-core_space space | {bottom top left right}]
  [-die_area {lx ly ux uy}]
  [-core_area {lx ly ux uy}]
  [-sites site_name]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-utilization` | percentage utilization (0-100) |
| `-aspect_ratio` | ratio $\frac{height}{width}$, default 1.0 |
| `-core_space` | space around core, default 0.0 microns. should be either one value for all margins or 4 values, one for each margin. |
| `-die_area` | die area coordinates in microns (lower left and upper right x-/y- coordinates) |
| `-core_area` | core area coordinates in microns (lower left and upper right x-/y- coordinates) |
| `-sites` | list of sites to make rows for |

The die area and core area used to write ROWs can be specified explicitly
with the `-die_area` and `-core_area` arguments. Alternatively, the die and
core areas can be computed from the design size and utilization as shown below:

Example computation:

```
core_area = design_area / (utilization / 100)
core_width = sqrt(core_area / aspect_ratio)
core_height = core_width * aspect_ratio
core = ( core_space_left, core_space_bottom )
      ( core_space_left + core_width, core_space_bottom + core_height )
die =  ( 0, 0 )
      ( core_width + core_space_left + core_space_right,
        core_height + core_space_bottom + core_space_top )
```

### Make Tracks

The `initialize_floorplan` command removes existing tracks. 

Use the `make_tracks` command to add routing tracks to a floorplan.

```tcl
make_tracks 
    [layer]
    [-x_pitch x_pitch]
    [-y_pitch y_pitch]
    [-x_offset x_offset]
    [-y_offset y_offset]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `layer` | select layer name to make tracks for (default all layers). |
| `-x_pitch`, `-y_pitch` | if set, overrides the LEF technology x-/y- pitch. |
| `-x_offset`, `-y_offset` | if set, overrides the LEF technology x-/y- offset. |

### Inserting tieoff cells

To insert tiecells:

```tcl
insert_tiecells 
    tie_pin
    [-prefix inst_prefix]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `tie_pin` | indicates the master and port to use to tie off nets. For example, `LOGIC0_X1/Z` for the Nangate45 library, where `LOGIC0_X1` is the master and `Z` is the output port on the master. |
| `-prefix` | used to control the prefix of the new tiecell names. This will default to `TIEOFF_`. |

### Useful developer functions

If you are a developer, you might find these useful. More details can be found in the [source file](./src/InitFloorplan.cc) or the [swig file](./src/InitFloorPlan.i).

```tcl
# convert microns to manufacturing grid dbu
microns_to_mfg_grid microns
```

## Example scripts

Example scripts on running InitFloorplan for a sample design of `mpd_top` are as follows:

```tcl
./test/upf_test.tcl
```

## Regression tests

There are a set of regression tests in `/test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following script:

```shell
./test/regression
```

## Limitations

## FAQs

Check out
[GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+ifp+in%3Atitle)
about this tool.

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
