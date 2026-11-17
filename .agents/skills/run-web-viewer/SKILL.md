---
name: run-web-viewer
description: Run, launch, drive, or screenshot the OpenROAD web viewer (src/web) in headless Chrome against a real design. Use to check a src/web change in the actual browser UI (timing, hierarchy, inspector, selection, layout, Tcl console panels), not just the jsdom unit tests.
---

# Run the OpenROAD web viewer

The viewer is served by `openroad` itself (`web_server`), with its JS/CSS
compiled into the binary. An agent drives it with three scripts in this
directory: `launch.sh` starts a server in the background, `driver.mjs`
runs a list of commands in one headless-Chrome session (click, drag,
Tcl, measure, screenshot), and `stop.sh` shuts the server down.

Run every command from the OpenROAD repo root; paths here, and relative
paths in a custom `load.tcl`, are relative to it.

## Prerequisites (one-time)

Google Chrome at `/usr/bin/google-chrome` (elsewhere, e.g. on macOS, set
`CHROME=` to its binary), and
`playwright-core` in a cache directory the driver looks in:

```bash
npm install --prefix ~/.cache/openroad-web-driver --no-audit --no-fund --silent playwright-core@1.48.2
```

## Build

The web assets are embedded in the binary: rebuild after every
`src/web` edit, or the browser shows the old code.

```bash
bazelisk build //:openroad
```

## Run (agent path)

```bash
O=/path/to/scratch/webrun          # new or empty: screenshots, logs, token
bash .agents/skills/run-web-viewer/launch.sh $O
node .agents/skills/run-web-viewer/driver.mjs $O \
  'shot 00-load' \
  'tab Timing' \
  'click .timing-widget button:text-is("Update")' \
  'wait 2000' \
  'widths .timing-path-table-container table' \
  'click .timing-path-table-container tbody tr' \
  'wait 500' \
  'drag .timing-detail-table-container thead th .col-resize-grip 80' \
  'widths .timing-detail-table-container table' \
  'shot 10-timing' \
  'tab Hierarchy' \
  'click .hierarchy-widget button:text-is("Update")' \
  'wait 1500' \
  'eval document.querySelectorAll(".hierarchy-table-container tbody tr").length' \
  'tab Inspector' \
  'clickxy 750 400' \
  'wait 1000' \
  'shot 20-inspector' \
  'tcl report_wns' \
  'shot 30-tcl'
bash .agents/skills/run-web-viewer/stop.sh $O
```

`launch.sh` prints the URL once the server listens on port 8765. `$O`
must be new, or an empty directory you own; it is made private (0700),
since each launch writes a random token to `$O/token`, where the driver
reads it. Use a fresh `$O` for each launch. The default design is the routed Nangate45 gcd from
`src/odb/test/data/gcd/`. Then **Read the PNGs** in `$O`.

`launch.sh` takes `WEB_PORT` (8765), `WEB_TIMEOUT` (seconds to wait for
the server, 60; raise it for a large design), and `OPENROAD_EXE` (the
binary, `bazel-bin/openroad`).

Driver commands (one argument each, run in order; the full list and
syntax are in the header of `driver.mjs`): `tab <Title>`,
`click <selector>`, `clickxy x y`, `shiftclickxy x y`,
`drag <selector> <dx>`, `hover`, `fill <selector> | <text>`, `key`,
`tcl <command>`, `wait <ms>`, `widths <table-selector>`, `eval <js>`,
`shot <name>`. `tcl` prints the console output the command added. The
first failing command prints `FAILED ...` and skips the rest. A page
that never lists its layers stops the run with `$O/load-failed.png`. Page
errors, console errors, failed HTTP requests, and that load failure are
printed (`PAGEERROR`, `CONSOLE`, `HTTP <status> <url>`,
`ERROR`). The driver exits 1 after any of these or a rejected token, so
exit 0 means the commands ran on a page that reported no errors.

`stop.sh` signals the recorded PID only while it is still the `openroad`
running `$O/web.tcl`, waits for it to exit, and removes the PID files.

### Another design

Pass a Tcl file that reads liberty, the database, and the SDC;
`launch.sh` appends `web_server`. `openroad` runs in the directory you
started `launch.sh` from (the repo root, as in every example here), so
relative paths in `load.tcl` resolve from there. Keep `load.tcl` outside
the new viewer's `out_dir`: a file already in it makes `launch.sh` refuse
the directory. Use `WEB_PORT` to run a second viewer next to the first.
For an ORFS result, with your own paths:

```tcl
read_liberty <platform>/lib/<stdcell>.lib
read_liberty <platform>/lib/<io-pads>.lib
read_db <results>/6_final.odb
read_sdc <results>/6_final.sdc
```

```bash
WEB_PORT=8766 bash .agents/skills/run-web-viewer/launch.sh $O/other $O/load.tcl
WEB_PORT=8766 node .agents/skills/run-web-viewer/driver.mjs $O/other 'tab Timing' \
  'click .timing-widget button:text-is("Update")' 'wait 3000' 'shot timing'
bash .agents/skills/run-web-viewer/stop.sh $O/other
```

## Unit tests

jsdom tests for most widgets run directly with `node`, which is faster
than Bazel, once `src/web/test/node_modules` (gitignored) exists:

```bash
(cd src/web/test && npm install --no-package-lock --no-audit --no-fund)
node --test src/web/test/js/test-timing-widget.js
bazelisk test //src/web/test/...      # all, including ones needing `three`
```

Bazel installs these dependencies from `src/web/test/pnpm-lock.yaml`,
which `npm` does not read; `--no-package-lock` keeps `npm` from writing
a `package-lock.json` (gitignored) that nothing else uses. An old
`node_modules` can hold an older jsdom than Bazel's; rerun the install
when a test passes under `node` but fails under Bazel, or the reverse.

## Gotchas

- **jsdom has no layout.** Every width and `getBoundingClientRect` is 0
  in the unit tests, so layout-dependent code only takes its fallback
  path there. Column sizing, clipping, and overflow need this skill.
- **`test/gcd_sky130hd.odb` is an unplaced netlist with no layers.**
  On it the layout is blank, Display Controls are empty, and the page
  logs `Failed to load initial data from server: TypeError ... '0'`.
  Use a placed or routed DEF/ODB.
- **The repo SDCs call `set_all_input_output_delays`,** a proc from
  `test/flow_helpers.tcl`; the default design sources it. A custom
  `load.tcl` using those SDCs needs `source test/flow_helpers.tcl` too.
- **ORFS SDCs name IO-pad cells** (`set_driving_cell -lib_cell
  sg13g2_IOPadIn`): read every liberty the design uses, not only the
  standard cells, or `read_sdc` fails and the server never starts.
- **Panel tabs live in `<ide-tab>` shadow roots.** Click them with
  `tab <Title>` (the tab's `title` attribute), not by text. A panel's
  content is not inside its `ide-tab`: select it by its widget class
  (`.timing-widget`, `.hierarchy-widget`, `.selection-browser`,
  `.inspector-highlight-btn`, ...).
- **The right-hand panels are about 340 px wide,** so wide tables scroll
  sideways and right-hand grips are off-screen. `drag` scrolls the
  element into view first; a raw `clickxy` on an off-screen element hits
  whatever is drawn there instead.
- **`clickxy` on the layout selects the top object there**, often a power
  net, not a cell. Check the Inspector's `Type` row in the screenshot.
- **The highlight palette** opens from `.inspector-highlight-btn`; groups
  are `.highlight-swatch[title="Group N"]`.
- **The server takes about 2 s to exit after SIGTERM**, and a new
  `launch.sh` on the same port fails with `WEB-0017 ... Address already
  in use`. `stop.sh` waits; `launch.sh` refuses a busy port up front.
- **Background the server with `exec`.** Without it, `( ... & )` leaves a
  `bash` parent around the server: its PID is not the server's, so
  killing it leaves the server running, and it holds the script's stdout,
  so `launch.sh | ...` never returns. `launch.sh` execs `openroad`, and
  `openroad` also writes its own `[pid]` to `server.pid`.
- **`OPENROAD` may already be set** in your environment to the repo
  directory, so the binary override is `OPENROAD_EXE`, as in ORFS.
- **The token guards a Tcl console that runs anything as you.** It is
  random per launch. `umask 077` alone does not protect it: it leaves an
  existing file's permissions alone and writes through a planted symlink.
  So `launch.sh` takes only a new or empty directory of yours, makes it
  0700, and writes with `noclobber`. Don't share the URL.
- **A recorded PID can be reused** by an unrelated process once the
  viewer exits; `stop.sh` checks the process's command line (`ps -o args=`,
  which macOS has too, unlike `/proc`) before signaling.
- **The display controls' first layer row is hidden,** so a wait for a
  *visible* `.vis-leaf` times out on a healthy page; the driver waits for
  the rows to exist.
- **Don't stop it with `pkill -f`/`pgrep -f` on a pattern** that also
  appears in your own command line: the pattern matches your shell and
  kills it. Use `stop.sh` (exact PID).
- A `401` for the favicon on every load is expected (the request carries
  no token); the driver skips it but reports any other failed request.

## Troubleshooting

| Symptom | Fix |
|---|---|
| `openroad exited; see .../or.log` with `invalid command name "set_all_input_output_delays"` | Source `test/flow_helpers.tcl` before `read_sdc`. |
| `or.log` ends in `set_driving_cell ... sg13g2_IOPadIn` | Read the IO-pad liberty too. |
| `port 8765 is in use` | `stop.sh` the old viewer, or set `WEB_PORT`. |
| The page shows the pre-edit UI | `bazelisk build //:openroad`, then relaunch. |
| `Cannot find module 'playwright-core'` | Run the prerequisite `npm install`, or point `PW_DIR` at an install. |
| Blank layout, a `TypeError ... '0'` console error, and `no layers listed after 30 s` | The design has no placement or tech layers; load a placed one. |
| `out_dir must be new, or an empty directory you own` | Pass a fresh `$O`; one from an earlier launch still holds its token and logs. Keep `load.tcl` outside it. |
| `no openroad binary at ...` | `bazelisk build //:openroad`, or point `OPENROAD_EXE` at the binary. Nothing was created, so rerun with the same `$O`. |
| `server did not start in 60 s` | Raise `WEB_TIMEOUT`; the server may still come up, so `stop.sh` it before relaunching. |
| `token rejected` | The driver read another launch's `$O/token`; pass the `out_dir` you launched with. |
