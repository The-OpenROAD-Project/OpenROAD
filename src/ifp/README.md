# Initialize Floorplan

This tool initializes floorplan constraints, die/core area, and makes tracks. 

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Initialize Floorplan

Do note that there are two ways of setting the floorplan dimensions.
The user can either specify manually die/core area, or
specify the utilization/aspect ratio.

#### Method 1: Automatic die size calculation

```tcl
initialize_floorplan
  [-utilization util]
  [-aspect_ratio ratio]
  [-core_space space | {bottom top left right}]
  [-sites site_name]
```
##### Options

| Switch Name | Description |
| ----- | ----- |
| `-utilization` | Percentage utilization. Allowed values are `double` in the range `(0-100]`. |
| `-aspect_ratio` | Ratio $\frac{height}{width}$. The default value is `1.0` and the allowed values are floats `[0, 1.0]`. |
| `-core_space` | Space around the core, default `0.0` microns. Allowed values are either one value for all margins or a set of four values, one for each margin. The order of the four values are: `{bottom top left right}`. |
| `-sites` | Tcl list of sites to make rows for (e.g. `{SITEXX, SITEYY}`) |


#### Method 2: Set die/core area

```tcl
initialize_floorplan
  [-die_area {llx lly urx ury}]
  [-core_area {llx lly urx ury}]
  [-sites site_name]
```

##### Options

| Switch Name | Description |
| ----- | ----- |
| `-die_area` | Die area coordinates in microns (lower left x/y and upper right x/y coordinates). |
| `-core_area` | Core area coordinates in microns (lower left x/y and upper right x/y coordinates). |
| `-sites` | Tcl list of sites to make rows for (e.g. `{SITEXX, ...}`) |

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
| `layer` | Select layer name to make tracks for. Defaults to all layers. |
| `-x_pitch`, `-y_pitch` | If set, overrides the LEF technology x-/y- pitch. Use the same unit as in the LEF file. |
| `-x_offset`, `-y_offset` | If set, overrides the LEF technology x-/y- offset. Use the same unit as in the LEFT file. |

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
| `tie_pin` | Indicates the master and port to use to tie off nets. For example, `LOGIC0_X1/Z` for the Nangate45 library, where `LOGIC0_X1` is the master and `Z` is the output port on the master. |
| `-prefix` | Used to control the prefix of the new tiecell names. This will default to `TIEOFF_`. |

### Useful developer functions

If you are a developer, you might find these useful. More details can be found in the [source file](./src/InitFloorplan.cc) or the [swig file](./src/InitFloorPlan.i).

| Command Name | Description |
| ----- | ----- |
| `microns_to_mfg_grid` | Convert microns to manufacturing grid DBU. |

## Example scripts

Example scripts on running `ifp` for a sample design of `mpd_top` are as follows:

```tcl
./test/upf_test.tcl
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
[GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+ifp+in%3Atitle)
about this tool.

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
