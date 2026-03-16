# DRC Destroyer — Arcade Game for Routing Visualization

This is just a fun feature and has no practical value.

## Motivation

This demo has a serious undertone on three levels:

1. **Democratization of EDA**: All of this was done automatically with zero
   programming. The source code for the OpenROAD project is the user-interface
   to Claude. Do what you need to do for your chip and use-case.

2. **Deployment use-case**: The game generates a standalone static HTML file —
   a single file with zero dependencies that anyone can open in a browser,
   share as a link, or attach to a PR. No servers, no installs, no runtime
   dependencies. This is how you ship tooling that people actually use.

3. **Speed**: This entire commit — game, tests, build targets, documentation —
   was created in ca. 30 minutes. Using real 3D (WebGL) instead of isometric
   (more 80s-idiomatic) would probably take a whole hour.

4. **Feedback loop with Claude**: The unit tests were not added out of vanity.
   They close the loop so Claude can quickly add extension points. When Claude
   needs to add a new ODB Python extension (e.g. for marker shape extraction),
   it writes the extension, adds a test, runs `bazelisk test`, sees the result,
   and iterates — all within a single conversation. The tests are the
   scaffolding that makes AI-assisted development fast and reliable.

## How to Run

```bash
# Play with demo data (no build required)
cd tools/OpenROAD
python3 test/orfs/mock-array/game.py --demo

# Play with real routing data (builds MockArray if needed)
bazelisk run //test/orfs/mock-array:game

# Run unit tests
bazelisk test //test/orfs/mock-array:test_game
```

## How It Works

`game.py` is a Python script that:

1. Reads the routed ODB database via the `odb` Python SWIG bindings
2. Extracts DRC markers, wire segments, die area, and cell instances
3. Parses log files for warnings and timing slack
4. Generates villain names from real build data
5. Writes a self-contained HTML/JS/Canvas arcade game

The generated HTML has zero dependencies — just open it in any browser.

## Python Extension Points

Claude generates these on the fly when requested. They are excluded from this
PR but documented here for the upgrade path.

### Currently Available (no new extensions needed)

| Feature | API | Status |
|---------|-----|--------|
| DRC violation bboxes + layer | `marker.getBBox()`, `marker.getTechLayer()` | Works |
| Violation type name | `category.getName()` | Works |
| Chip die area | `block.getDieArea()` | Works |
| Wire segments + layer | `dbWireDecoder` PATH/POINT opcodes | Works |
| Layer names | `dbTechLayer.getName()` | Works |
| Cell instance names | `inst.getMaster().getName()` | Works |

### Extension Points for Game Upgrade

**1. Marker shape extraction** (`src/odb/src/swig/common/dbhelpers.i`):
`marker.getShapes()` returns `std::variant` which SWIG can't handle.
Add a helper that converts shapes to Python-friendly format.

**2. Marker source identification** (same file):
`marker.getSources()` returns `std::set<dbObject*>`. Add a helper to
identify source types (net, inst, etc.) as strings.

**3. Bulk wire extraction** (same file):
For performance, add a C++ helper that extracts all wire segments as
flat arrays instead of decoding one-by-one in Python.

## Maintainer: Build and Release

Anyone can play the game by clicking the link in a GitHub release.

```bash
# 1. Build the game
cd tools/OpenROAD
bazelisk run //test/orfs/mock-array:game

# 2. Create a release with the HTML attached
gh release create game-v1.0 \
  --title "DRC Destroyer" \
  --notes "Fly over the chip and drop bombs on DRC violations!
Download drc_destroyer.html and open in any browser.
This is just a fun feature and has no practical value." \
  /tmp/drc_destroyer.html

# Alternative: Upload as PR artifact in CI
# - uses: actions/upload-artifact@v4
#   with:
#     name: drc-destroyer
#     path: /tmp/drc_destroyer.html

# Alternative: Host on GitHub Pages
# Copy drc_destroyer.html to docs/ and enable Pages.
```
