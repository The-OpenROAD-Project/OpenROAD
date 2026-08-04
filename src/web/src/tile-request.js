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
// The server renders each tile at 256*dpr physical pixels while Leaflet lays it
// out at 256 CSS px, so the image maps 1:1 onto the device pixel grid and the
// browser never resamples it — which is what would otherwise reintroduce the
// moiré beat on HiDPI displays.
//
// The display's real ratio is honoured, clamped to [1,3] and rounded to two
// decimals. The rounding exists only to bound the server's tile-cache key space
// (dpr is part of the key), not to force the tile onto a fixed ladder: the ratio
// decides the rendered tile's pixel size, so a display whose ratio got snapped
// would receive a tile of the wrong size and have the browser rescale it —
// the moiré beat the tile pipeline is built to avoid. 1.75 and 2.5 are ordinary
// Windows scale factors and a ladder mis-serves both.
//
// MUST match quantizeDpr() in request_handler.cpp. The merged path sizes its
// canvas backing store from this and draws into it at that size, so any
// disagreement resamples every tile AND makes the memory budget wrong — it
// would reserve one bitmap size while holding another.
export function quantizeDpr(raw) {
    if (!Number.isFinite(raw) || raw <= 1) {
        return 1;
    }
    return Math.round(Math.min(raw, 3) * 100) / 100;
}

export function nativeDpr() {
    const dpr = (typeof window !== 'undefined' && window.devicePixelRatio)
        ? window.devicePixelRatio
        : 1;
    return quantizeDpr(dpr);
}

// Pure builder for the tile-request payload. `ctx` carries the per-layer
// visibility/selectability context.
//
// Tiles don't use selectability for rendering, but it is sent on every request
// so the wire schema stays uniform with selectAt requests.
//
// `dpr` is passed in rather than read here so a caller can override it without
// this module knowing anything about why.
export function buildTileRequestFor(coords, layerName, ctx, dpr) {
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
        dpr,
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

// Floor the map's REAL (possibly fractional) zoom so the displayed tile pane is
// only ever UPSCALED (scale 2^(realZoom-floor) ∈ [1,2)), never downscaled.
// Upscaling a band-limited tile cannot reintroduce the moiré beat; downscaling
// can. Reading the live map zoom (not the passed arg) keeps this correct during
// zoom-animation frames and robust even if zoomSnap:0 is re-enabled.
export function floorClampZoom(layer, zoom) {
    const real = (layer && layer._map) ? layer._map.getZoom() : zoom;
    return Math.floor(real);
}
