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
viewer in the default browser. The command blocks until the server stops, so
the design can be explored interactively; Tcl commands typed in the browser
console run on the server.

```tcl
web_server
    [-port port]
    [-stop]
```

#### Options

| Switch Name | Description |
| ---------- | -------------------------------------------------- |
| `-port` | TCP port to listen on. Default: `0`, which picks a free port. |
| `-stop` | Stop a running server and return from the blocked `web_server` call. |
| `-dir` | Deprecated and ignored; the web assets are embedded in the binary. |

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
| -------------- | ---------------------------------------------- |
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
| `rudy` | false | Estimated congestion (RUDY) heatmap overlay |

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

# Save with RUDY congestion heatmap overlay
save_image -web -display_option {rudy true} layout_rudy.png
```

### Save Animated GIF

Build an animated GIF from a sequence of layout snapshots, so an optimization
loop can be watched frame by frame.

The command is a three-call state machine. `-start` opens a stream and returns
an integer key; each `-add` captures the current state of the design as one
frame; `-end` finalizes the file. Several streams can be open at once, told
apart by their keys.

This is the same command the Qt GUI provides, and it dispatches to whichever
viewer is up: the Qt GUI grabs its own window, while the web renderer
composites the frame server-side, needing neither a display nor a running
server — so it also covers a Qt build launched without `-gui`.
`-display_option` is web-only, so on the Qt path it warns and is ignored.

```tcl
save_animated_gif
    (-start|-add|-end)
    [-area {x0 y0 x1 y1}]
    [-width width]
    [-resolution microns_per_pixel]
    [-delay delay]
    [-key key]
    [-display_option option]
    [path]
```

#### Options

| Switch Name | Description |
| ----------- | ------------------------------------------------- |
| `-start` | Open a new GIF stream and return its key. Requires `path`; the frame options are ignored. |
| `-add` | Capture the design's current state as one frame. Takes no `path`. |
| `-end` | Finalize and close the GIF. Takes no `path`. |
| `-area` | Bounding box in microns `{x0 y0 x1 y1}`. The default is the die area with a 5% margin. |
| `-width` | Frame width in pixels. The type is `int`, and must be positive. Cannot be used with `-resolution`. The default is `1024`. |
| `-resolution` | Resolution in microns per pixel. The type is `float`, and must be positive; it is raised to 1 DBU per pixel if finer. Cannot be used with `-width`. |
| `-delay` | Time each frame is shown, in hundredths of a second. The type is `int`, and must be positive. The default is `250`, i.e. 2.5 seconds. |
| `-key` | Which open stream to act on, as returned by `-start`. The type is `int`. The default is the most recently opened stream. |
| `-display_option` | Repeatable visibility overrides as `{control value}` pairs, the same keys `save_image -web` accepts. See [Display option keys](#display-option-keys-web-mode) above. |
| `path` | Output GIF file path. Required with `-start`, rejected otherwise. |

The defaults above are the web renderer's; on the Qt path the GUI's own apply
— `-area` defaults to what is visible on screen, and `-resolution` to the
zoom the GUI is at.

On the web path, the first frame fixes the GIF's dimensions; a later frame that
comes out a different size — because the design's bounding box grew, say — is
rescaled to match rather than starting a second GIF. Area outside the design is
left transparent, which most viewers show as black. Ending a stream that never
received a frame writes no file and warns. The maximum frame dimension is 16384
pixels, as for `save_image`; larger requests are clamped.

#### Examples

```tcl
# One GIF, one frame per placement iteration
set gif [save_animated_gif -start placement.gif]
for {set i 0} {$i < 10} {incr i} {
  global_placement -incremental
  save_animated_gif -add -key $gif -width 800 -delay 50
}
save_animated_gif -end -key $gif

# A single stream needs no key
save_animated_gif -start route.gif
save_animated_gif -add -resolution 0.1
save_animated_gif -add -resolution 0.1
save_animated_gif -end

# Zoom in on a region and show only routing
save_animated_gif -add -area {0 0 100 100} \
                       -display_option {stdcells false} \
                       -display_option {routing true}
```

### Save Report

Generate a self-contained HTML timing report. The report uses the same
JavaScript frontend as the live web viewer but serves all data from a cache
embedded in the HTML file. No running server is required to view the report.

```tcl
web_save_report
    [-setup_paths count]
    [-hold_paths count]
    path
```

#### Options

| Switch Name | Description |
| ----------- | ------------------------------------------------- |
| `-setup_paths` | Maximum number of setup timing paths to include. Default: `100`. |
| `-hold_paths` | Maximum number of hold timing paths to include. Default: `100`. |
| `path` | Output HTML file path. |

The report includes:

- **Layout view** with pre-rendered tiles at a fixed zoom level. Layer
  visibility can be toggled using the same display controls as the live viewer.
  Zoom is disabled; pan is allowed.
- **Timing table** with setup and hold paths. Clicking a path highlights it
  on the layout via a pre-rendered overlay image.
- **Slack histogram** with setup/hold tabs.
- **Display controls**, hierarchy browser, clock tree, and other panels from
  the live viewer (features that require server interaction show empty states).

The report is self-contained: Leaflet, GoldenLayout and three are inlined as
`data:` URIs, so it opens from `file://` with no server and no network.  The
schematic panel is not available in it -- it needs the server, and its two
libraries would add 2.8 MB to every saved file.

#### Examples

```tcl
# Generate a report with default settings
web_save_report timing.html

# Include more paths
web_save_report -setup_paths 200 -hold_paths 200 timing.html
```

### Save Display Controls

Write the display-controls state of the connected viewer to a JSON file, so a
particular set of visibility, selectability, layer-pattern and theme choices
can be reused later or shared.

```tcl
save_display_controls
    filename
```

#### Options

| Switch Name | Description |
| ----------- | ------------------------------------------------- |
| `filename` | Output JSON file path. |

The viewer pushes a snapshot of its state to the server whenever a control
changes, and this command writes the most recent snapshot. It therefore
requires a running server (`web_server`) with a viewer open; otherwise it
errors, or warns if no snapshot has arrived yet. The server caches a single
snapshot, so with several viewers connected the state saved is that of
whichever one synced last.

Because Tcl commands typed into the browser console run on the server, this is
normally invoked from the viewer's own console after arranging the panel.

### Restore Display Controls

Apply a previously saved display-controls file to every connected viewer.

```tcl
restore_display_controls
    filename
```

#### Options

| Switch Name | Description |
| ----------- | ------------------------------------------------- |
| `filename` | Input JSON file path, as written by `save_display_controls`. |

The state is broadcast to all connected clients, each of which re-applies it
and reloads, preserving the current camera position. A key absent from the file
is reset to its default rather than left as-is. The file is validated before it
is broadcast; a malformed or unexpectedly shaped file is rejected with an error
and nothing is applied.

Like `save_display_controls`, this requires a running server.

#### Examples

```tcl
# In the viewer's Tcl console, after setting up the panel
save_display_controls my_view.json

# Later, in another session with the viewer open
restore_display_controls my_view.json
```

### Create Toolbar Button

Add a button to the viewer's toolbar that runs a Tcl script when clicked.

Returns the button's key, either `name` or `buttonN`.

This is the same command the Qt GUI provides, and it dispatches to whichever
viewer is up: the Qt GUI when its window is running, the web viewer otherwise
— including in a Qt build launched without `-gui`.

```tcl
create_toolbar_button
    [-name name]
    -text button_text
    -script tcl_script
    [-icon icon]
    [-tooltip tooltip]
    [-toggle]
    [-script_off tcl_script_off]
    [-echo]
```

#### Options

| Switch Name | Description |
| ----------- | ------------------------------------------------- |
| `-name` | Key used to remove the button later. The type is `string`. The default is a generated `buttonN`. Re-using a registered name is an error. |
| `-text` | Label to put on the button. The type is `string`. Required. |
| `-script` | Tcl script to evaluate when the button is clicked, or — with `-toggle` — when it is switched on. The type is `string`. Required. |
| `-icon` | Icon to show before the label: either literal text (an emoji, say) or an image referenced by `data:` URI, `http(s):` URL or a path starting with `/` or `./`. The type is `string`. The default is no icon. |
| `-tooltip` | Hover text. The type is `string`. The default is no tooltip. |
| `-toggle` | Make the button a two-state switch: clicking it runs `-script` when turning it on and `-script_off` when turning it off. |
| `-script_off` | Tcl script to evaluate when a `-toggle` button is switched off. The type is `string`. Ignored without `-toggle`. |
| `-echo` | Echo the script into the browser's Tcl console before running it. |

`-icon`, `-tooltip`, `-toggle` and `-script_off` are web-only: the Qt GUI's
buttons are text-only and stateless, so when the dispatch lands on Qt these
warn and are ignored rather than erroring, letting one script drive either
viewer.

### Remove Toolbar Button

Remove a button added by `create_toolbar_button`. Removing a name that is not
registered does nothing.

```tcl
remove_toolbar_button
    name
```

#### Options

| Switch Name | Description |
| ----------- | ------------------------------------------------- |
| `name` | Key of the button to remove, as returned by `create_toolbar_button`. The type is `string`. |

### Create Menu Item

Add an item to the viewer's menu bar that runs a Tcl script when chosen.

Returns the item's key, either `name` or `actionN`.

This is the same command the Qt GUI provides, and it dispatches to whichever
viewer is up: the Qt GUI when its window is running, the web viewer otherwise
— including in a Qt build launched without `-gui`.

```tcl
create_menu_item
    [-name name]
    [-path menu_path]
    -text item_text
    -script tcl_script
    [-shortcut shortcut]
    [-echo]
```

#### Options

| Switch Name | Description |
| ----------- | ------------------------------------------------- |
| `-name` | Key used to remove the item later. The type is `string`. The default is a generated `actionN`. Re-using a registered name is an error. |
| `-path` | Menu to place the item in, as a `/`-separated hierarchy. The first segment is a top-level menu, created if it does not exist; deeper segments nest as submenus. The type is `string`. The default is `Custom Scripts`. |
| `-text` | Text to put on the item. The type is `string`. Required. |
| `-script` | Tcl script to evaluate when the item is chosen. The type is `string`. Required. |
| `-shortcut` | Key shortcut that triggers the item, e.g. `Ctrl+H`. Displayed beside the label. The type is `string`. The default is none. |
| `-echo` | Echo the script into the browser's Tcl console before running it. |

`-shortcut` is written the way Qt writes it: modifiers (`Ctrl`, `Shift`,
`Alt`, `Meta` — `Cmd` is accepted for `Meta`) joined to a key with `+`, in any
order. On the web path the key is matched as the browser reports it, so name
non-printing keys accordingly: `Escape`, `ArrowUp`, `F5`. A spec naming no key
(`Ctrl+`) or an unrecognized modifier is ignored rather than bound to
something the script did not ask for, and a shortcut is not delivered while a
text field has focus. A custom shortcut that claims a key the viewer already
uses takes it over, rather than triggering both.

### Remove Menu Item

Remove an item added by `create_menu_item`. Removing a name that is not
registered does nothing.

```tcl
remove_menu_item
    name
```

#### Options

| Switch Name | Description |
| ----------- | ------------------------------------------------- |
| `name` | Key of the item to remove, as returned by `create_menu_item`. The type is `string`. |

#### Examples

```tcl
# A button that reports the current worst slack
create_toolbar_button -text "Worst slack" \
                      -script {report_worst_slack} \
                      -icon "⏱" \
                      -tooltip "Report the worst setup slack" \
                      -echo

# A two-state button, running one script when switched on and another off
create_toolbar_button -name drt_debug -text "DRT maze" -toggle \
                      -script {set_debug_level DRT maze 1} \
                      -script_off {set_debug_level DRT maze 0}
remove_toolbar_button drt_debug

# A menu item under a submenu of its own
create_menu_item -name checkpoint -path "Flow/Checkpoints" \
                 -text "Write DB..." -shortcut "Ctrl+S" \
                 -script {write_db checkpoint.odb}
remove_menu_item checkpoint
```

On the web path these can be run before `web_server`, from a startup script:
the registry lives on the server, so it survives page reloads and is served to
clients that connect later. Each change is also pushed to every connected
client, so a button created in one browser's Tcl console appears in all of
them — something the single-window Qt GUI cannot do.

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
  rows, tracks). The panel state can be saved to and restored from a file with
  `save_display_controls` / `restore_display_controls`.
- **Focus nets** — Isolate specific nets for inspection, dimming all other
  routing.
- **Tcl console** — Execute Tcl commands interactively from the browser.
- **Custom menu items and toolbar buttons** — Bind Tcl scripts to the viewer's
  menu bar, toolbar and keyboard with `create_menu_item` /
  `create_toolbar_button`. The registry lives on the server, so it is shared by
  every connected browser.
- **Image export** — Save the layout as a PNG with `save_image -web` or as an
  animated GIF with `save_animated_gif`, both headless. Panels export
  themselves client-side, from a button in the panel rather than a Tcl command:
  the schematic as SVG or PNG, the 3D view as PNG, the charts as CSV or PNG,
  and the clock tree as PNG.
- **Editing utilities** — Inspect, delete and re-apply global-connect rules,
  and insert a buffer on a net, from dialogs in the Tools menu and the
  inspector.

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

## Server API

The browser↔server protocol (WebSocket message types, request fields,
response shapes, error contract) is documented in
[`docs/server-api.md`](docs/server-api.md).

## Example scripts

```tcl
# Start the web viewer on an OS-assigned port
web_server

# Start on a fixed port
web_server -port 8080
```

## Regression tests

There are a set of regression tests in `./test`.

```shell
bazel test //src/web/test/...
```

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
