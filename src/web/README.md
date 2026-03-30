# Web Viewer

The web viewer module in OpenROAD (`web`) provides a browser-based interface
for exploring chip layouts and performing design analysis. It renders the design
as PNG tiles served over WebSocket, enabling smooth zoom and pan of large
designs without a native GUI.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Web Server

Start the web viewer server. This opens a WebSocket server and launches the
viewer in the default browser.

```tcl
web_server
    [-port port]
    -dir dir
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-port` | TCP port to listen on. Default: `8080`. |
| `-dir` | Path to the document root directory containing the web assets (`index.html`, `*.js`, `*.css`). |

### Save Image

Save the layout to a PNG file. By default, the command uses the GUI (Qt)
renderer. Pass `-web` to use the web tile renderer instead, which runs
entirely server-side without a display and is suitable for headless CI.

```tcl
save_image
    [-web]
    [-area {x0 y0 x1 y1}]
    [-width width]
    [-resolution microns_per_pixel]
    [-display_option option]
    path
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-web` | Use the web tile renderer instead of the GUI renderer. Does not require a display or a running web server. |
| `-area` | Bounding box in microns `{x0 y0 x1 y1}`. Default: die area (with 5% margin in `-web` mode). |
| `-width` | Output image width in pixels. Cannot be used with `-resolution`. |
| `-resolution` | Resolution in microns per pixel. Minimum: 1 DBU per pixel. Cannot be used with `-width`. |
| `-display_option` | Repeatable visibility overrides as `{control value}` pairs. See [Display option keys](#display-option-keys) below. |
| `path` | Output PNG file path. |

When using `-web`, if neither `-width` nor `-resolution` is specified, the
image defaults to 1024 pixels wide. The maximum image dimension is 16384
pixels; larger requests are clamped automatically.

#### Display option keys (web mode)

Display options control which elements are rendered when using `-web`.
Each option is a `{key value}` pair where the key matches a visibility
field and the value is `true` or `false`.

| Key | Default | Description |
| --- | ------- | ----------- |
| `stdcells` | true | Standard cells |
| `macros` | true | Macros |
| `routing` | true | Signal routing |
| `special_nets` | true | Power/ground straps |
| `pins` | true | Instance pins |
| `pin_markers` | true | IO pin direction markers |
| `blockages` | true | Blockages |
| `net_signal` | true | Signal nets |
| `net_power` | true | Power nets |
| `net_clock` | true | Clock nets |
| `rows` | false | Row outlines |
| `tracks_pref` | false | Preferred-direction tracks |

#### Examples

```tcl
# Save using the GUI renderer (default)
save_image layout.png

# Save using the web renderer (headless)
save_image -web layout.png

# Save at 1024px wide with the web renderer
save_image -web -width 1024 layout.png

# Save at 0.1 um per pixel
save_image -web -resolution 0.1 layout.png

# Save a specific region (in microns)
save_image -web -area {0 0 100 100} -width 2048 region.png

# Hide routing and power nets
save_image -web -display_option {routing false} \
                -display_option {net_power false} \
                layout.png
```

## Features

- **Tile-based rendering** — The server renders 256x256 PNG tiles on demand,
  supporting smooth zoom and pan of designs with millions of instances.
- **Object inspection** — Click on instances, nets, pins, or other objects to
  view their properties in an inspector panel. Hover highlights are rendered
  server-side in tiles.
- **Timing analysis** — View timing paths with slack, delay, and arrival time
  metrics. Highlight critical paths on the layout. Slack histogram charts with
  filtering by path group and clock domain.
- **Clock tree visualization** — Browse clock tree hierarchy, highlight clock
  paths, and view per-level statistics.
- **Hierarchy browser** — Navigate the module tree with instance counts and area
  statistics. Toggle visibility and assign colors per module using a 31-color
  palette.
- **Display controls** — Toggle visibility of cell types (stdcells, macros,
  pads), net types (signal, power, clock), and shapes (routing, pins, blockages,
  rows, tracks).
- **Focus nets** — Isolate specific nets for inspection, dimming all other
  routing.
- **Tcl console** — Execute Tcl commands interactively from the browser.

## Architecture

The module has two parts:

- **C++ server** (`src/web.cpp`, `src/request_handler.cpp`,
  `src/tile_generator.cpp`) — A Boost Beast WebSocket server that handles tile
  rendering, object selection, timing/clock-tree queries, and Tcl evaluation.
  Tiles are rendered from ODB geometry and encoded as PNG using lodepng.

- **JavaScript frontend** (`src/main.js`, `src/index.html`, `src/style.css`) —
  A single-page application using Leaflet.js for the map and GoldenLayout for
  resizable panels. Communicates with the server over a binary WebSocket
  protocol.

## Example scripts

```tcl
# Start the web viewer, pointing at the web assets directory
web_server -dir /path/to/OpenROAD/src/web/src
```

## Regression tests

There are a set of regression tests in `./test`.

```shell
bazel test //src/web/test/...
```

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
