# Web Viewer Server API

This document describes the wire protocol between the OpenROAD web
viewer's browser frontend and its C++ server. The protocol is intended
to be stable enough that an alternate client (custom browser app,
headless test driver, third-party visualization) can be implemented
against it without reading the JavaScript reference frontend.

The server exposes one HTTP route for static assets and one WebSocket
endpoint for all dynamic operations. The WebSocket carries a small
binary framing with JSON or PNG payloads.

## Connecting

- HTTP and WebSocket share one TCP port (default `8080`, configured via
  `web_server -port`). A single Boost.Beast listener serves both.
- HTTP `GET /` returns `index.html`; `GET /<path>` returns the embedded
  asset at that path (`*.js`, `*.css`, etc.). 404 otherwise. No method
  other than GET is supported.
- The WebSocket upgrade is also at the root path. Once upgraded, all
  request/response traffic uses the binary framing below.

## Wire frame format

Every WebSocket message — both client→server and server→client — is a
binary frame with the following layout:

| Bytes | Field    | Type                | Description                           |
| ----- | -------- | ------------------- | ------------------------------------- |
| 0..3  | `id`     | `uint32` big-endian | Request correlator (see *Correlation*). |
| 4     | `type`   | `uint8`             | Payload type: `0=JSON`, `1=PNG`, `2=Error`. |
| 5..7  | reserved | `uint8 × 3`         | Must be zero.                         |
| 8..   | `payload`| bytes               | UTF-8 JSON, PNG-encoded image, or UTF-8 error string. |

### Correlation

- The client picks a 32-bit `id` for each request and the server echoes
  it on the corresponding response. The reference frontend uses a
  monotonically-increasing counter; any unique value is fine.
- Server-push messages (broadcasts that aren't replies to a specific
  request) use `id = 0`.

### Payload types

- `0 = JSON` — body is a UTF-8 JSON document. Default response shape.
- `1 = PNG` — body is a raw PNG file. Used by `tile` and `heatmap_tile`.
- `2 = Error` — body is a UTF-8 plain-text error message. Surfaces in
  the server log as `WEB-0043` (see *Error contract* below).

## Request envelope

Every request from the client is a JSON object with at least:

| Field  | Type     | Required | Description                                 |
| ------ | -------- | :------: | ------------------------------------------- |
| `id`   | `int`    |    ✓     | Request id; echoed back unmodified.         |
| `type` | `string` |    ✓     | Selects the handler. See per-type sections. |

The rest of the object's fields are handler-specific.

A request with malformed JSON, a non-object root, or missing/wrongly-typed
`id` or `type` produces a kError response of the form
`Malformed request (missing or invalid id/type)` and a `WEB-0043`
warning. A well-formed request whose `type` is not registered produces
`Unknown request type: <type>` and the same warning.

## Wire conventions

The server is **strict on field types**. Every field below has one
canonical JSON type; sending any other type for a present field yields a
`kError` response.

- Integers travel as JSON numbers parsed as `int64`.
- Booleans travel as JSON `true` / `false`.
- Doubles are JSON numbers; whole-valued numbers may parse as integers
  on the wire, which the server tolerates for double-typed fields only.
- Strings are JSON strings; arrays are JSON arrays.
- Two narrow exceptions documented inline:
  - `select.zoom` accepts `int` or `double` (Leaflet's `zoomSnap: 0`
    permits fractional zoom values).
  - `set_heatmap.value` for `int`-typed settings accepts `int` or
    `double` and rounds (the JS frontend always uses `parseFloat`).

A "✓" in the **Required** column means the server reads the field
unconditionally. Missing fields throw and produce a `kError`.

---

## Server-push messages

These arrive with `id = 0` and `type = JSON`. The client subscribes by
simply listening on the WebSocket; there's no opt-in.

| `type` field   | Trigger                                                                 | Body                            |
| -------------- | ----------------------------------------------------------------------- | ------------------------------- |
| `refresh`      | Initial spatial-index build finished after a connection / `web_server`. | `{"type":"refresh"}`            |
| `log`          | Buffered server log lines being flushed to clients.                     | `{"type":"log","text":"<lines>"}` |
| `debug_paused` | The graphics-debug pause hook entered (e.g. placer breakpoint).         | `{"type":"debug_paused"}`       |
| `debug_resumed`| Pause hook exited.                                                      | `{"type":"debug_resumed"}`      |
| `debug_refresh`| Debug overlay needs a re-render.                                        | `{"type":"debug_refresh"}`      |
| `shutdown`     | Server is exiting; client should not auto-reconnect.                    | `{"type":"shutdown"}`           |

---

## Tile rendering

### `tile`

Render a single 256×256 PNG tile of the layout.

| Field            | Type     | Required | Description |
| ---------------- | -------- | :------: | ----------- |
| `layer`          | `string` |    ✓     | Layer name (`metal1`, `metal2`, …) or one of the synthetic layers `_instances`, `_modules`, `_pins`, or `_overlay`. |
| `z`              | `int`    |    ✓     | Leaflet tile zoom level (0 = whole design). |
| `x`              | `int`    |    ✓     | Tile column at zoom `z`. |
| `y`              | `int`    |    ✓     | Tile row at zoom `z`. |
| `visible_layers` | `string[]` | ✓ | Tech layers currently visible on the *pins* synthetic layer; empty array means hide all pin markers. |
| *visibility flags* | `bool` |  per-flag default | See *TileVisibility flags* below. Any flag may be omitted to take the default. |
| `site_<name>`    | `bool`   |    —     | Per-row-site visibility. Only consulted when `rows == true`. |

**Response:** PNG image (frame type `1`).

### `bounds`

Return the design's bounding box and shape-cache readiness.

No request fields beyond the envelope.

**Response (JSON):**
```json
{
  "bounds": [[yMin, xMin], [yMax, xMax]],
  "shapes_ready": true,
  "pin_max_size": 200
}
```

`bounds` is in DBU. `shapes_ready` indicates whether the spatial index
finished building (the `refresh` push fires when this flips to true).
`pin_max_size` is the largest BPin dimension in DBU, used by the client
to size the pin-marker overlay.

### `tech`

Return tech-layer metadata, sites, and block info.

No request fields.

**Response (JSON):**
```json
{
  "layers": ["metal1", "via1", "metal2", "..."],
  "layer_colors": [[r, g, b], ...],
  "sites": ["FreePDK45_38x28_10R_NP_162NW_34O", "..."],
  "has_liberty": true,
  "dbu_per_micron": 1000,
  "block_name": "top"
}
```

`layer_colors` is parallel to `layers`; each entry is RGB in `0..255`.
`block_name` is the empty string when no block is loaded.

---

## Selection and inspection

### `select`

Pick the topmost selectable object at a DBU coordinate. Cycles through
overlapping objects on repeated calls at the same point.

| Field            | Type     | Required | Description |
| ---------------- | -------- | :------: | ----------- |
| `dbu_x`          | `int`    |    ✓     | X coordinate in DBU. |
| `dbu_y`          | `int`    |    ✓     | Y coordinate in DBU. |
| `zoom`           | `int` or `double` | ✓ | Current Leaflet zoom level. Server truncates fractional values. |
| `visible_layers` | `string[]`|    ✓    | Same semantics as `tile`. |
| *visibility flags* | `bool` |  per-flag default | Same as `tile`. |
| `site_<name>`    | `bool`   |    —     | Same as `tile`. |

**Response (JSON):** the inspect payload (see `inspect` below) plus a
top-level `selected` array listing every overlapping candidate:

```json
{
  "selected": [
    {"name": "buf1", "type": "Inst", "bbox": [xMin, yMin, xMax, yMax]},
    ...
  ],
  "can_navigate_back": 0,
  "name": "buf1",
  "type": "Inst",
  "properties": [...],
  "bbox": [xMin, yMin, xMax, yMax]
}
```

When no object is selectable at the point, `selected` is `[]` and only
`can_navigate_back` is set (no `name`/`type`/`properties`).

### `inspect`

Re-inspect a previously-selected object using its session-local id from
a prior `select`.

| Field       | Type  | Required | Description                                            |
| ----------- | ----- | :------: | ------------------------------------------------------ |
| `select_id` | `int` |    ✓     | ID returned in a previous `select` `*_select_id` field. |

**Response (JSON):** same shape as the inspect portion of `select`'s
response, *without* the `selected` array:

```json
{
  "can_navigate_back": 1,
  "name": "buf1/Z",
  "type": "ITerm",
  "properties": [
    {"name": "Master", "value": "BUF_X16",
     "value_select_id": 12},
    {"name": "Children", "children": [
      {"name": "child", "value": "...", "value_select_id": 13},
      ...
    ]}
  ],
  "bbox": [xMin, yMin, xMax, yMax],
  "has_guides": 1
}
```

`*_select_id` fields appear next to any property whose value is itself
selectable; the client passes them back via `inspect` to drill down.
`has_guides` is present (and `1`) only when the inspected object is a
net with route guides. `can_navigate_back` is `1` whenever the
back-navigation history is non-empty.

If `select_id` is out of range, the response includes
`{"error": "invalid select_id"}` instead of the metadata.

### `inspect_back`

Pop the back-navigation stack and re-emit the inspect payload for the
previous object. No request fields.

**Response (JSON):** same shape as `inspect` but for the previous object.

### `hover`

Highlight an object's shapes (mouseover preview) without selecting it.

| Field       | Type  | Required | Description                                                |
| ----------- | ----- | :------: | ---------------------------------------------------------- |
| `select_id` | `int` |    ✓     | Selectable id; pass `-1` to clear hover (mouseleave).      |

**Response (JSON):**
```json
{"ok": 1, "count": 3, "rects": [[xMin, yMin, xMax, yMax], ...]}
```

`count` is the number of highlight rects collected; an empty rect array
falls back to the object bbox.

### `snap`

Find the nearest design edge to a cursor position (for the ruler /
crosshair tool).

| Field             | Type        | Required | Description |
| ----------------- | ----------- | :------: | ----------- |
| `dbu_x`           | `int`       |    ✓     | Cursor X in DBU. |
| `dbu_y`           | `int`       |    ✓     | Cursor Y in DBU. |
| `radius`          | `int`       |    ✓     | Search radius in DBU. |
| `point_threshold` | `int`       |    ✓     | If two snap candidates fall within this many DBU of each other, prefer the point endpoint. |
| `horizontal`      | `bool`      |    ✓     | Allow snapping to horizontal edges. |
| `vertical`        | `bool`      |    ✓     | Allow snapping to vertical edges. |
| `visible_layers`  | `string[]`  |    ✓     | Layers to consider for routing snaps. |
| *visibility flags*| `bool`      |  per-flag default | Same as `tile`. |

**Response (JSON):**
```json
{
  "found": true,
  "is_point": false,
  "edge": [[x1, y1], [x2, y2]]
}
```

`is_point` is `true` when the snap collapsed to a point (e.g. corner).
On `"found": false`, `edge` and `is_point` are absent.

---

## Schematic

### `schematic_cone`

Build a cone of influence around an instance (fanin/fanout) and return
it as a Yosys-format netlist for the netlistsvg renderer.

| Field           | Type     | Required | Description                                |
| --------------- | -------- | :------: | ------------------------------------------ |
| `inst_name`     | `string` |    ✓     | Anchor instance name.                      |
| `fanin_depth`   | `int`    |    ✓     | BFS depth upstream (`0` = no fanin).        |
| `fanout_depth`  | `int`    |    ✓     | BFS depth downstream.                       |

**Response (JSON):** a Yosys netlist JSON
([schema](https://yosyshq.readthedocs.io/projects/yosys/en/latest/cmd/write_json.html))
limited to one module named `top`:

```json
{
  "modules": {
    "top": {
      "attributes": {},
      "ports":    {"<bterm name>": {"direction": "input", "bits": [n]}},
      "cells":    {"<inst name>":  {"hide_name": 0, "type": "<master>", "attributes": {}, "parameters": {}, "port_directions": {...}, "connections": {...}}},
      "netnames": {"<net name>":   {"hide_name": 0, "bits": [n], "attributes": {}}}
    }
  }
}
```

Cones are capped at 150 instances; nets with fanout > 30 are skipped to
keep the schematic readable.

### `schematic_full`

Same shape as `schematic_cone` but emits the entire block (no caps). No
request fields.

### `schematic_inspect`

Inspect a single instance by name (used when the schematic widget
selects a cell). Equivalent to a `select`+`inspect` pair but skips the
geometric pick.

| Field       | Type     | Required | Description                  |
| ----------- | -------- | :------: | ---------------------------- |
| `inst_name` | `string` |    ✓     | Instance name to inspect.    |

**Response (JSON):** same shape as `inspect`.

---

## Tcl

### `tcl_eval`

Evaluate a Tcl command. Captures `info` log output and the command's
result.

| Field | Type     | Required | Description           |
| ----- | -------- | :------: | --------------------- |
| `cmd` | `string` |    ✓     | The Tcl source string. |

**Response (JSON):**
```json
{
  "output":  "captured stdout/log lines",
  "result":  "Tcl_GetStringResult(interp)",
  "is_error": false
}
```

When the user runs `exit` or `quit` from the browser, the server's
override turns the result into a shutdown signal:

```json
{
  "output":   "...",
  "result":   "Exiting OpenROAD.",
  "is_error": false,
  "action":   "shutdown"
}
```

Clients that see `action: "shutdown"` should disable auto-reconnect.

### `tcl_complete`

Tab-completion for the Tcl prompt. Splits the line at the cursor and
reports candidate completions plus the replacement range.

| Field        | Type     | Required | Description                              |
| ------------ | -------- | :------: | ---------------------------------------- |
| `line`       | `string` |    ✓     | Full input line.                         |
| `cursor_pos` | `int`    |    ✓     | Cursor offset into `line` (0-based).     |

**Response (JSON):**
```json
{
  "completions":   ["...", "..."],
  "mode":          "commands" | "variables" | "arguments",
  "prefix":        "the matched prefix",
  "replace_start": 4,
  "replace_end":   8
}
```

---

## Timing and charts

### `timing_report`

Return the worst N timing paths.

| Field        | Type     | Required | Default                          | Description                                          |
| ------------ | -------- | :------: | -------------------------------- | ---------------------------------------------------- |
| `is_setup`   | `bool`   |    ✓     | —                                | `true` → setup paths, `false` → hold.                |
| `max_paths`  | `int`    |    ✓     | —                                | Maximum number of paths to return.                   |
| `slack_min`  | `double` |          | `-FLT_MAX`                       | Lower slack bound (inclusive). Optional filter.      |
| `slack_max`  | `double` |          | `+FLT_MAX`                       | Upper slack bound (exclusive). Optional filter.      |

**Response (JSON):**
```json
{
  "paths": [
    {
      "start_clk": "clk",  "end_clk": "clk",
      "required":  1.0,    "arrival": 1.5,
      "slack":    -0.5,    "skew":    0.0,
      "path_delay": 0.9,   "logic_depth": 4, "fanout": 12,
      "start_pin":  "ff1/CK", "end_pin": "ff2/D",
      "data_nodes":    [{"pin": "...", "fanout": 1, "rise": true,  "clk": false, "time": 0.0, "delay": 0.0, "slew": 0.0, "load": 0.0}, ...],
      "capture_nodes": [{...}]
    },
    ...
  ]
}
```

### `timing_highlight`

Highlight one timing path's shapes on the layout, optionally with a
single stage emphasized.

| Field        | Type     | Required when            | Description                                                    |
| ------------ | -------- | ------------------------ | -------------------------------------------------------------- |
| `path_index` | `int`    | always                   | Index into the most recent `timing_report.paths`. `-1` clears. |
| `is_setup`   | `bool`   | `path_index >= 0`        | Which side of the report `path_index` indexes into.            |
| `pin_name`   | `string` | optional, `>= 0` only    | If set, emphasize this pin's net within the path.              |

**Response (JSON):** `{"ok": true}`. The actual update is the layer
overlay redraw on the next `tile` request.

### `slack_histogram`

Histogram of endpoint slacks for one side of the timing report.

| Field        | Type     | Required | Description                                                |
| ------------ | -------- | :------: | ---------------------------------------------------------- |
| `is_setup`   | `bool`   |    ✓     | Setup vs hold endpoints.                                    |
| `path_group` | `string` |          | If non-empty, restrict to this path group. Default: all.    |
| `clock_name` | `string` |          | If non-empty, restrict to this clock. Default: all.         |

**Response (JSON):**
```json
{
  "bins": [{"lower": -1.0, "upper": 0.0, "count": 12, "negative": true}, ...],
  "unconstrained_count": 5,
  "total_endpoints":    1234,
  "time_unit":          "ns"
}
```

### `chart_filters`

List the path groups and clocks available for the slack histogram
filter dropdowns. No request fields.

**Response (JSON):**
```json
{"path_groups": ["**default**", ...], "clocks": ["clk1", "clk2", ...]}
```

### `clock_tree`

Compute the clock tree (CTS view).

No request fields.

**Response (JSON):**
```json
{
  "clocks": [
    {
      "name": "clk",
      "min_arrival": 0.0,
      "max_arrival": 0.5,
      "time_unit":   "ns",
      "nodes": [
        {"id": 0, "parent_id": -1, "name": "...", "pin_name": "...",
         "type": "root" | "buffer" | "inverter" | "clock_gate" | "register" | "macro" | "unknown",
         "arrival": 0.0, "delay": 0.0, "fanout": 0, "level": 0,
         "dbu_x": 0, "dbu_y": 0},
        ...
      ]
    }
  ]
}
```

### `clock_tree_highlight`

Highlight a single clock-tree node's instance in the layout.

| Field       | Type     | Required | Description                              |
| ----------- | -------- | :------: | ---------------------------------------- |
| `inst_name` | `string` |    ✓     | Empty string clears the highlight.        |

**Response (JSON):** `{"ok": true}`.

---

## Module hierarchy

### `module_hierarchy`

Return the module hierarchy tree (left sidebar).

No request fields.

**Response (JSON):**
```json
{
  "nodes": [
    {
      "id": 0, "parent_id": -1,
      "inst_name":   "<top>", "module_name": "...",
      "insts":     1234, "macros":      0, "modules":     5,
      "area":     1.23,
      "local_insts": 100, "local_macros": 0, "local_modules": 5,
      "node_kind":   1,           // present when not kModule (kLeafGroup=1, kTypeGroup=2, kInstance=3)
      "odb_id":      42,           // present only on kModule nodes
      "color":       [r, g, b]     // present only on kModule nodes
    },
    ...
  ]
}
```

### `set_module_colors`

Update the per-module color override map (for the `_modules` tile
layer).

| Field    | Type     | Required | Description                                                                                  |
| -------- | -------- | :------: | -------------------------------------------------------------------------------------------- |
| `colors` | `string` |    ✓     | Custom delimited form: `<id>:<r>,<g>,<b>,<a>;<id>:<r>,<g>,<b>,<a>;...`. Empty = clear all. |

**Response (JSON):** `{"ok": 1, "count": <updated module count>}`.

### `set_focus_nets`

Add or remove a net from the focus-nets set (route-tracing overlay).

| Field      | Type     | Required | Description                                            |
| ---------- | -------- | :------: | ------------------------------------------------------ |
| `action`   | `string` |    ✓     | `"add"`, `"remove"`, or `"clear"`.                     |
| `net_name` | `string` |    ✓     | Net name for `add`/`remove`. Pass `""` for `clear`; the value is unused but the field must be present. |

**Response (JSON):** `{"ok": 1, "count": <focus-net count>}`.

### `set_route_guides`

Same shape as `set_focus_nets` but updates the route-guides overlay.

---

## Heat maps

### `heatmaps`

Return metadata for every registered heat map.

No request fields.

**Response (JSON):**
```json
{
  "active":  "Pin",                    // empty when none active
  "heatmaps": [
    {
      "name":               "Pin",
      "title":              "Pin Density",
      "active":             true,
      "settings_group":     "Density",
      "has_data":           true,
      "can_adjust_grid":    true,
      "show_numbers":       false,
      "show_legend":        true,
      "supports_numbers":   true,
      "units":              "/um²",
      "display_range_increment": 1.0,
      "display_min":        0.0,    "display_max":       100.0,
      "display_min_limit":  0.0,    "display_max_limit": 100.0,
      "draw_below_min":     true,   "draw_above_max":    true,
      "log_scale":          false,  "reverse_log":       false,
      "grid_x":             10.0,   "grid_y":           10.0,
      "grid_min":           1.0,    "grid_max":         1000.0,
      "alpha":              150,    "alpha_min":        0,    "alpha_max": 255,
      "bounds":             [xMin, yMin, xMax, yMax],
      "options":            [...],   // source-specific extra settings
      "legend":             [{"value": "5.0", "color": [r, g, b, a]}, ...]
    },
    ...
  ]
}
```

### `set_active_heatmap`

Activate a heat map (or none).

| Field   | Type     | Required | Description                            |
| ------- | -------- | :------: | -------------------------------------- |
| `name`  | `string` |    ✓     | Empty string deactivates the current.   |

**Response (JSON):** same shape as `heatmaps`.

### `set_heatmap`

Update one setting on one heat map.

| Field    | Type     | Required | Description |
| -------- | -------- | :------: | ----------- |
| `name`   | `string` |    ✓     | Heat-map name (e.g. `"Pin"`). |
| `option` | `string` |    ✓     | Setting key (e.g. `"DisplayMin"`, `"Alpha"`, `"ShowNumbers"`); also `"rebuild"` to force a re-population. |
| `value`  | `bool`, `int`, `double`, or `string` | ✓ when `option != "rebuild"` | Type must match the setting's variant slot, with one tolerance: int-typed settings accept JSON doubles and round (the JS frontend always uses `parseFloat`). |

**Response (JSON):** same shape as `heatmaps`.

### `heatmap_tile`

Render one tile for the active heat map (or a specified one).

| Field  | Type     | Required | Description                                                |
| ------ | -------- | :------: | ---------------------------------------------------------- |
| `name` | `string` |    ✓     | Heat-map name; empty string ⇒ use the active heat map.     |
| `z`    | `int`    |    ✓     | Tile zoom. |
| `x`    | `int`    |    ✓     | Tile column. |
| `y`    | `int`    |    ✓     | Tile row. |

**Response:** PNG (frame type `1`).

---

## DRC

### `drc_categories`

List top-level DRC categories.

No request fields.

**Response (JSON):**
```json
{
  "categories": [
    {"name": "DRC", "count": 42, "description": "...", "source": "..."},
    ...
  ]
}
```

`description` and `source` are present only when the category provides
them.

### `drc_markers`

Drill into one DRC category and return its subcategories + markers.

| Field      | Type     | Required | Description                                                                       |
| ---------- | -------- | :------: | --------------------------------------------------------------------------------- |
| `category` | `string` |    ✓     | Top-level category name. Empty string clears the active category and returns `{"subcategories":[]}`. |

**Response (JSON):**
```json
{
  "name":        "DRC",
  "total_count": 42,
  "subcategories": [
    {
      "name":  "Subcat",
      "count": 3,
      "subcategories": [...],
      "markers": [
        {
          "id":     1,
          "index":  1,
          "name":   "...",
          "visited": false, "visible": true, "waived": false,
          "bbox":   [xMin, yMin, xMax, yMax],
          "layer":  "metal1",         // optional
          "comment": "rule X",        // optional
          "sources": [{"type": "Net" | "Inst" | "ITerm" | "BTerm" | "Object", "name": "..."}, ...]
        },
        ...
      ]
    },
    ...
  ]
}
```

When the named category isn't found, the response is
`{"error": "Category not found: <name>"}`.

### `drc_load_report`

Load DRC markers from a `.rpt`, `.drc`, or `.json` file on disk.

| Field  | Type     | Required | Description                                                  |
| ------ | -------- | :------: | ------------------------------------------------------------ |
| `path` | `string` |    ✓     | Server-side filesystem path. Format inferred from extension. |

**Response (JSON):** `{"ok": 1, "category": "DRC", "count": 42}` on
success or `{"ok": 0, "error": "..."}` on no-violations / failure.

### `drc_update_marker`

Toggle one marker's `visited` or `visible` flag.

| Field       | Type     | Required | Description                                          |
| ----------- | -------- | :------: | ---------------------------------------------------- |
| `marker_id` | `int`    |    ✓     | Marker id from `drc_markers` `markers[].id`.         |
| `field`     | `string` |    ✓     | `"visited"` or `"visible"`.                          |
| `value`     | `bool`   |    ✓     | New value.                                           |

**Response (JSON):** `{"ok": 1, "id": <id>, "field": <field>, "value": <bool>}`.

### `drc_update_category_visibility`

Bulk-set every marker in a category to a single `visible` value.

| Field      | Type     | Required | Description           |
| ---------- | -------- | :------: | --------------------- |
| `category` | `string` |    ✓     | Top-level name.       |
| `visible`  | `bool`   |    ✓     | New visibility.       |

**Response (JSON):** `{"ok": 1, "category": "...", "visible": <bool>, "count": <updated>}`.

### `drc_highlight`

Center the layout viewport on a marker and set the highlight rect.

| Field       | Type  | Required | Description                                            |
| ----------- | ----- | :------: | ------------------------------------------------------ |
| `marker_id` | `int` |    ✓     | Marker id; pass `-1` to clear the highlight.           |

**Response (JSON):** on hit:
```json
{"ok": 1, "bbox": [xMin, yMin, xMax, yMax], "name": "...", "visited": true, "layer": "metal1"}
```

`layer` is omitted when the marker has no tech layer. On miss (or
explicit clear with `-1`): `{"ok": 0}`.

---

## Files

### `list_dir`

Server-side directory listing (used by the file-open dialog).

| Field  | Type     | Required | Description                                                              |
| ------ | -------- | :------: | ------------------------------------------------------------------------ |
| `path` | `string` |    ✓     | Absolute path. Empty string ⇒ current working directory of the server. |

**Response (JSON):**
```json
{
  "path":   "/canonical/absolute",
  "parent": "/canonical",
  "entries": [
    {"name": "subdir",    "is_dir": true},
    {"name": "report.rpt","is_dir": false, "size": 12345},
    ...
  ]
}
```

Hidden entries (`.`-prefixed) and unreadable entries are skipped.
Directories are listed before files; both groups sort alphabetically.

---

## Debug graphics

### `debug_continue`

Resume execution after a debug-pause hook (`gui::Gui::pause()` etc.).
Inline-dispatched (does not enter the worker thread pool). No request
fields.

**Response (JSON):** `{"ok":1}`.

### `debug_charts`

Snapshot the registered debug charts. Inline-dispatched. No request
fields.

**Response (JSON):**
```json
{
  "charts": [
    {
      "name":     "...",
      "x_label":  "iteration",
      "y_labels": ["loss", "violations"],
      "x_format": "{}",  "y_formats": ["{:.2e}", "{}"],
      "points":   [{"x": 0, "ys": [0.5, 12]}, ...]
    },
    ...
  ]
}
```

---

## TileVisibility flags

These flags appear together (typically prefixed with `vf` in the JS
client) on every request that takes a viewport: `tile`, `select`, and
`snap`. All are `bool`. Each handler defaults a missing flag to its
struct default — the column below — but the JS reference frontend
always sends the full set.

| Field                  | Default | Notes |
| ---------------------- | :-----: | ----- |
| `stdcells`, `macros`   | `true`  | Instance categories (Liberty-aware when STA is loaded). |
| `pad_input`, `pad_output`, `pad_inout`, `pad_power`, `pad_spacer`, `pad_areaio`, `pad_other` | `true` | Pad sub-types. |
| `phys_fill`, `phys_endcap`, `phys_welltap`, `phys_tie`, `phys_antenna`, `phys_cover`, `phys_bump`, `phys_other` | `true` | Physical-only cell sub-types. |
| `std_bufinv`, `std_bufinv_timing`, `std_clock_bufinv`, `std_clock_gate`, `std_level_shift`, `std_sequential`, `std_combinational` | `true` | Std-cell sub-types (need Liberty/STA). |
| `net_signal`, `net_power`, `net_ground`, `net_clock`, `net_reset`, `net_tieoff`, `net_scan`, `net_analog` | `true` | By `dbSigType`. |
| `routing`, `routing_segments`, `routing_vias`, `special_nets`, `srouting_segments`, `srouting_vias` | `true` | Wires & vias. |
| `pins`, `pin_markers`, `pin_names` | `true` | BTerm shapes & labels. |
| `inst_names`, `inst_pins`, `inst_pin_names` | `true` | ITerm shapes & labels. |
| `blockages`, `placement_blockages`, `routing_obstructions` | `true` | dbBlockage / dbObstruction. |
| `rows`                 | `false` | Enables the row-outline overlay; gates `site_<name>` lookup. |
| `tracks_pref`, `tracks_non_pref` | `false` | Routing-track overlay. |
| `debug`, `debug_renderers`, `debug_live` | `false` | Debug graphics overlay. |
| `site_<name>` (multi-key) | `false` | Per-row-site visibility. Only consulted when `rows == true`. |

---

## Error contract

The server treats wire-protocol violations as recoverable: the offending
request gets a `kError` response, the rest of the session continues.

- **Malformed message** (invalid JSON, non-object root, missing/wrong-typed
  `id` or `type`): `kError` body
  `Malformed request (missing or invalid id/type)`.
- **Unknown type**: `kError` body `Unknown request type: <type>`.
- **Handler exception** (boost::json `at()` on a missing field, type
  conversion failure, internal `runtime_error`, …): `kError` body
  `server error: <exception what()>`.

Every `kError` response logs a server-side warning:

```
[WARNING WEB-0043] request id=<id> type=<type> failed: <body>
```

`type` is the original `type` string from the request when it was
parseable, otherwise `unknown`. The full request payload is *not*
logged; clients should reproduce locally if a payload is needed for
diagnosis.

The reference JS client surfaces `kError` responses as a Promise
rejection through `WebSocketManager.request(...)`. Custom clients
should handle frame type `2` similarly.
