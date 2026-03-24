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
