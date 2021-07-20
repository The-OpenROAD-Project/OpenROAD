# Initialize Floorplan

```
initialize_floorplan
  [-site site_name]               LEF site name for ROWS
  -die_area "lx ly ux uy"         die area in microns
  [-core_area "lx ly ux uy"]      core area in microns
or
  -utilization util               utilization (0-100 percent)
  [-aspect_ratio ratio]           height / width, default 1.0
  [-core_space space
    or "bottom top left right"]   space around core. Should either be one value
                                  for all margins or 4 values for each margin.
                                  default 0.0 (microns)
```

The die area and core size used to write ROWs can be specified explicitly
with the -die_area and -core_area arguments. Alternatively, the die and
core area can be computed from the design size and utilization as show below:

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

The `initialize_floorplan` command removes existing tracks.
Use the `make_tracks` command to add routing tracks to a floorplan.

```
make_tracks [layer]
            [-x_pitch x_pitch]
            [-y_pitch y_pitch]
            [-x_offset x_offset]
            [-y_offset y_offset]
```

With no arguments `make_tracks` adds X and Y tracks for each routing layer.
With a `-layer` argument `make_tracks` adds X and Y tracks for layer with
options to override the LEF technology X and Y pitch and offset.

Place pins around core boundary.

```
auto_place_pins pin_layer
```
