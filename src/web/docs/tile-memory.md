# Tile memory and layer grouping

Why the viewer composites several tech layers into one tile pane instead of
giving each its own.

## The failure

On a 3DBlox multi-die design, large parts of the layout went white when the
pointer passed over the display controls, and stayed white until something
forced a full invalidation — a window resize, or opening DevTools. The tiles
themselves were fine throughout: every one Leaflet held was loaded, sized and
still decodable.

## Cause

The viewer created one Leaflet pane per (tech layer x chiplet instance). That
design had 97 panes, each holding a full grid of ~24 tiles, each tile decoding
to `(256*dpr)^2 * 4` bytes — **582 MB of decoded images at dpr 1**. Past roughly
458 MB on that machine, Chrome discards decoded images, and the discarded
regions paint white.

A single-die design shows ~20 panes, so ~120 MB. That is the whole difference
between "works for everyone else" and "unusable".

## Evidence

Measured by replacing tile images with a 1x1 pixel — which removes the bytes
while leaving the pane count, the DOM and the composited-layer structure
untouched — and by forcing the tile dpr up and down. All runs at 97 panes, with
`bad=0/2328` tiles, so none of this is tile delivery:

| Decoded image bytes | content-bearing panes | Whiteout |
|---|---|---|
| 0 MB | 0/97 | no |
| 456 MB | 76/97 | no |
| 462 MB | 77/97 | marginal, slight flicker |
| 480 MB | 80/97 | yes |
| 516 MB | 86/97 | yes |
| 517 MB | **97/97** | yes |
| 582 MB (baseline) | 97/97 | yes |
| ~5238 MB (dpr 3) | 97/97 | worse |

Monotonic in bytes across seven points. The 516-vs-517 pair is the one that
identifies the mechanism: identical bytes, 86 versus 97 content-bearing panes,
identical failure. So at constant bytes the *distribution* is irrelevant, which
rules out both "too many panes" and "too many full-coverage panes stacked per
pixel". It is the bytes.

Also ruled out by measurement, not argument: tile delivery, decode failure,
dropped blobs behind revoked URLs, tile positioning, stale Leaflet map size,
clipping, GPU rasterization (it reproduces under `--disable-gpu`), compositor
raster memory (white at 228 MB estimate, clean at 385 MB), JS heap (20-30 MB of
4192 MB), and main-thread blocking.

One ambiguity was never resolved: removing the bytes also removes the work to
raster them, so this does not separate a memory cap from raster/decode
throughput. Both point the same way for the fix.

## The fix

Decoded image bytes are

```
bytes = panes x tilesPerPane x (256 x dpr)^2 x 4
```

so bounding them means bounding `panes`. The routing layers are partitioned into
N groups, each rasterized into one canvas per tile position, with N derived from
a memory budget (default 350 MB, ~23% under the measured ceiling):

```
N = budget / (tilesPerPane x bytesPerTile)
```

`tilesPerPane` is recomputed from the viewport, which matters: at 2560x1440 a
pane holds ~77 tiles rather than 24, and 97 panes there would be ~1.9 GB. A
fixed N chosen at one window size would fail on a maximised 4K display.

`_instances`, `_pins` and `_modules` keep their own panes — three of them, no
meaningful memory, and their own add/remove rules.

### Invariants

- **Groups must be contiguous runs of the z-order.** Each becomes one image, and
  two images stack correctly only if neither interleaves with the other: for
  z-order `[1,2,3,4]`, the grouping `{1,3},{2,4}` has no stacking that
  reproduces the original.
- **The merged pane's own opacity must be 1.** Leaflet applies `options.opacity`
  to the layer container, so per-layer opacity (0.7 for routing layers, 1 for
  the rest) now lives inside the canvas. A translucent pane would apply it twice
  — 0.49 — and wash the view out.
- Grouping is exact because src-over is associative, *provided* the intermediate
  keeps its alpha and is composited at alpha 1. `test-tile-merge.js` checks this
  numerically at every split point.
- **Client and server must agree on dpr to the pixel.** The merged path sizes its
  canvas backing store from its own copy, so a mismatch resamples every tile and
  makes the budget wrong. `quantizeDpr` is mirrored in `tile-request.js` and
  `request_handler.cpp`.

### Options

```
?mergetiles=0      one pane per layer (the previous behaviour)
?tilebudget=<MB>   override the budget
?mergegroups=<N>   pin N directly
```

Startup logs `[tiles] merged N layers into M panes (~X MB of Y MB budget,
T tiles/pane)`, and the same figures are on `app.mergeStats`.

## What this does not fix

- **Bandwidth.** The same K images are still fetched per tile; only a
  server-side composite would change that.
- **Request count.** A refresh still queues thousands of requests. Larger tiles
  would help there — but not with memory: four 256px tiles and one 512px tile
  are the same pixels.
- **Empty tiles.** A blank tile still decodes to a full-size transparent bitmap.
  Suppressing those server-side would cut real memory, though by an amount that
  depends entirely on the design (85% of tiles were blank on the design measured
  here, potentially none on a dense one).
