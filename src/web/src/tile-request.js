// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Wire format for tile requests, shared by the per-layer tile layer and the
// merged (multi-layer) one.
//
// Split out of websocket-tile-layer.js so both layer types share one definition
// of the payload and cannot drift on the wire — a divergence would give the two
// paths different server-side tile-cache keys, and one of them would miss the
// cache entirely.

// Normalize the display's device pixel ratio into the value sent on the wire.
//
// The pixel size of a tile no longer comes from this: `tile_px` carries the
// exact device-pixel square the tile will occupy (see buildTileRequestFor), so
// the image maps 1:1 onto the device grid and the browser never resamples it —
// which is what would otherwise reintroduce the moiré beat on HiDPI displays.
// Rounding the ratio and multiplying by the tile size, as this once did, cannot
// express 426.67 device px and so guaranteed a resample on any display whose
// scale is not a neat fraction of the tile size.
//
// What the ratio still decides is how pixel-authored sizes (fonts, stroke
// widths, the sub-resolution cull) scale, which tolerates the two-decimal
// rounding — its purpose is to bound the server's tile-cache key space, since
// dpr is part of the key.
//
// MUST match quantizeDpr() in request_handler.cpp.
export function quantizeDpr(raw) {
    if (!Number.isFinite(raw) || raw <= 1) {
        return 1;
    }
    return Math.round(Math.min(raw, 3) * 100) / 100;
}

// CSS size of one tile, chosen so tileSize*dpr is a WHOLE number of device
// pixels for every ratio a display reports in practice.
//
// 256 is not such a number. At a 1.6667 display scale (166%, ordinary on
// Windows and Linux) 256 CSS px is 426.67 device px, so the tile *pitch* is
// fractional: boundaries land at 426.67k device px and only every third one can
// sit on the grid, whatever the pane offset is. The browser antialiases the
// other two into their neighbours, src-over leaves the coverage short of 1, and
// the pane background shows through as a dark hairline on every seam.
//
// 240 = 2^4 * 3 * 5 is divisible by the denominator of every real ratio —
// 1.25 = 5/4, 1.5 = 3/2, 1.6667 = 5/3, 1.75 = 7/4, 1.375 = 11/8, 1.1 = 11/10,
// 2.5 = 5/2 — so tileSize*dpr comes out whole and the pitch is exact.
//
// Total decoded bytes barely move: the tiles are 6% smaller on a side and there
// are ~14% more of them, so the pixel count is a wash (see tile-merge.js for the
// budget). The cost is the extra request per tile.
export const TILE_SIZE_CSS = 240;

// Static reports carry tiles pre-rasterized at 256 CSS px by whoever authored
// them. Those are files, not requests, so that path keeps the old size.
export const STATIC_TILE_SIZE_CSS = 256;

// The CSS tile size in force for this session.  Resolved once rather than
// passed around: the map's coordinate scale, all three tile layers and the
// request payload must agree on it, and a value threaded through five call
// sites is a value that eventually disagrees with itself.
let tile_size_css = TILE_SIZE_CSS;

export function tileSizeCss() {
    return tile_size_css;
}

// Switch to the static-report tile size.  Called once at startup, before any
// layer is built, when the viewer is showing a baked report instead of a live
// design: those tiles are 256 px images embedded in the file (web.cpp renders
// them at the default dpr), so any other box resamples every one of them.
// Reports are usually read at dpr 1, where 256 CSS px is exactly 1:1.
export function useStaticTileSize() {
    tile_size_css = STATIC_TILE_SIZE_CSS;
}

// The default, named so the choice reads as a pair rather than a one-way door.
export function useDeviceExactTileSize() {
    tile_size_css = TILE_SIZE_CSS;
}

// Merge the device-exact tile size into a Leaflet options object without
// overriding an explicit choice (the static path passes its own).
export function withDeviceExactTileSize(options) {
    const opts = { ...(options || {}) };
    if (opts.tileSize === undefined) {
        opts.tileSize = tileSizeCss();
    }
    return opts;
}

// Device pixels for one tile: what the server is asked to render, and what a
// merged canvas sizes its backing store to. Sent explicitly rather than left to
// the server to derive, so the image is exactly the box it will be drawn into.
export function tileDevicePx(css_tile_size, dpr) {
    return Math.max(1, Math.round(css_tile_size * dpr));
}

// The display's REAL ratio, unrounded.
//
// Everything that sizes something works in real device pixels — the requested
// pixel count, the merged canvas backing store — because rounding first and
// multiplying after cannot land on the box: at 1.6666666 rounded to 1.67, a 240
// CSS px tile asks for 401 px to fill 400, and the browser resamples every tile
// to make up the difference.  Quantization applies to the `dpr` FIELD only,
// where its job is to bound the server's cache-key space.
export function nativeDpr() {
    const raw = (typeof window !== 'undefined' && window.devicePixelRatio)
        ? window.devicePixelRatio
        : 1;
    if (!Number.isFinite(raw) || raw <= 1) {
        return 1;
    }
    // Clamped, not rounded.  The clamp is a memory guard — a tile is rendered
    // at (tileSize*dpr*supersample)^2, so an absurd ratio from a deeply zoomed
    // browser would allocate hundreds of megabytes for one tile.  Rounding, on
    // the other hand, is exactly what stops the image fitting its box.
    return Math.min(raw, 3);
}

// The sizing fields every tile request carries, whatever kind it is.
//
// Layer, overlay and heat-map tiles are stacked on top of each other in the
// browser, so all three have to be rendered at the same pixel count and land on
// the same grid; an overlay half a pixel off, or an image the browser has to
// rescale, shows up as a blurred or misregistered highlight.
export function tileSizeFields(dpr, tileSize = tileSizeCss()) {
    return {
        // Rounded here and nowhere else: this field feeds the server's cache
        // key and its CSS-authored sizes (fonts, pen widths), both of which
        // tolerate two decimals.  `tile_px` must NOT be derived from it.
        dpr: quantizeDpr(dpr),
        // The exact device-pixel square this tile will be displayed in, from
        // the REAL ratio.  The server renders this count rather than deriving
        // one, because neither end can derive it: 256*dpr at 1.6667 is 426.67,
        // a size no image can have, and rounding the ratio first gives 401 for
        // a 400 px box.  Either way the browser resamples every tile.
        tile_px: tileDevicePx(tileSize, dpr),
    };
}

// Pure builder for the tile-request payload. `ctx` carries the per-layer
// visibility/selectability context.
//
// Tiles don't use selectability for rendering, but it is sent on every request
// so the wire schema stays uniform with selectAt requests.
//
// `dpr` is passed in rather than read here so a caller can override it without
// this module knowing anything about why.
export function buildTileRequestFor(coords, layerName, ctx, dpr,
                                    tileSize = tileSizeCss()) {
    const { visibility, selectability, visibleLayers, selectableLayers, app }
        = ctx;
    const vf = {};
    for (const [k, v] of Object.entries(visibility || {})) {
        vf[k] = !!v;
    }
    if (selectability) {
        for (const [k, v] of Object.entries(selectability)) {
            vf['s_' + k] = !!v;
        }
    }
    const req = {
        type: 'tile',
        layer: layerName,
        z: coords.z,
        x: coords.x,
        y: coords.y,
        ...tileSizeFields(dpr, tileSize),
        visible_layers: visibleLayers ? [...visibleLayers] : [],
        selectable_layers: selectableLayers ? [...selectableLayers] : [],
        ...vf,
    };
    if (app && app.visibleChiplets instanceof Set) {
        req.visible_chiplets = [...app.visibleChiplets];
    }
    // Per-layer fill pattern (int matching the server's FillPattern enum;
    // 1 = solid). Read lazily so a change via the layer context menu is picked
    // up on the next refresh without rebuilding the layer.
    if (app && app.layerPatterns) {
        const p = app.layerPatterns[layerName];
        if (p !== undefined && p !== 1) {
            req.pattern = p;
        }
    }
    return req;
}

// Call `onChange` whenever devicePixelRatio changes — a window dragged to
// another monitor, or the browser zoom altered.
//
// Nothing else notices: tiles are requested at the ratio in force when they were
// created and are never revisited, so after a change the viewer keeps displaying
// images rasterized for the old one, stretched into the new box. A tile fetched
// at 1.25 and shown at 1.6667 is a 33% upscale — every edge smeared into its
// neighbour, which reads as thick, soft seams over the whole layout.
//
// A resolution media query is the only reliable signal (there is no dpr event);
// each query only ever matches the ratio it was built for, so a fresh one is
// installed after every change. Returns an unsubscribe function.
export function watchDevicePixelRatio(onChange) {
    if (typeof window === 'undefined' || typeof window.matchMedia !== 'function'
        || typeof onChange !== 'function') {
        return () => {};
    }
    let stopped = false;
    let query = null;
    let handler = null;
    const listen = () => {
        if (stopped) {
            return;
        }
        const dpr = window.devicePixelRatio || 1;
        query = window.matchMedia(`(resolution: ${dpr}dppx)`);
        handler = () => {
            if (stopped) {
                return;
            }
            listen();
            onChange(window.devicePixelRatio || 1);
        };
        // `once` so the handler cannot fire twice for one change; listen()
        // installs the next one against the new ratio.
        query.addEventListener('change', handler, { once: true });
    };
    listen();
    return () => {
        stopped = true;
        // `once` retires a handler that has fired, but the one waiting on the
        // current ratio has not: without this it outlives the caller, and a
        // viewer that is mounted and torn down repeatedly accumulates them.
        if (query && handler) {
            query.removeEventListener('change', handler);
        }
        query = null;
        handler = null;
    };
}

// Floor the map's REAL (possibly fractional) zoom so the displayed tile pane is
// only ever UPSCALED (scale 2^(realZoom-floor) ∈ [1,2)), never downscaled.
// Upscaling a band-limited tile cannot reintroduce the moiré beat; downscaling
// can. Reading the live map zoom (not the passed arg) keeps this correct during
// zoom-animation frames and robust even if zoomSnap:0 is re-enabled.
export function floorClampZoom(layer, zoom) {
    const real = (layer && layer._map) ? layer._map.getZoom() : zoom;
    return Math.floor(real);
}
