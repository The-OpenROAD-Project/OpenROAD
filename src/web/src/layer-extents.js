// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Which layer tiles are worth requesting.
//
// Most of a technology's layers hold nothing in any given view: implant and
// front-end layers are absent from cell abstracts, upper metals go unused by
// the block, and the sub-resolution cull empties the rest at zoom-out.  Each of
// those tiles still costs the client a request, a reply and a paint, and on a
// zoom those costs, not the server's rendering, set how long the view takes.
// The server's layer_extents response bounds where each tech layer can have
// content; a tile outside its layer's box is not requested at all.
//
// Correctness rests on never skipping a tile that has something to draw:
//   - Until extents arrive, and after every invalidate(), nothing is skipped.
//   - The owner invalidates on every "refresh" push (the server's signal that
//     the design changed) and fetches again; a reply to a fetch issued before
//     the latest invalidate() is dropped, since it may predate the edit.
//   - Layers the server did not list (pseudo layers) are always requested.
//   - Sources gated by a visibility flag (master pins and obstructions,
//     routing obstructions, fills) have extents of their own, which count
//     unless that flag is explicitly off -- a flag the request leaves out takes
//     the server's default, and those are all on but fills.
//   - Tracks and the debug overlays draw on layer tiles without being design
//     geometry, so skipping stops while any of them is on.

// Visibility flags under which a layer tile can carry content that is not in
// the extents.
const NON_GEOMETRY_FLAGS = [
    'tracks_pref', 'tracks_non_pref', 'debug', 'debug_renderers',
];

// Fraction of a tile by which its box is grown before testing it against a
// layer's extent.  The renderer draws an apron of a few pixels around every
// tile and grows sub-pixel shapes to a minimum size, so a shape just outside
// the tile can still put pixels inside it; 1/16 of a tile is several times
// either.
const TILE_MARGIN = 1 / 16;

// Whether tile `coords` ({x, y, z}) can show anything of `extent`, a
// [x0, y0, x1, y1] box on the zoom-0 tile grid.  Null is no extent at all; a
// malformed one is treated as covering everything.
function tileOverlaps(extent, coords) {
    if (extent == null) {
        return false;
    }
    if (!Array.isArray(extent) || extent.length !== 4 || !coords) {
        return true;
    }
    const n = Math.pow(2, coords.z);
    const margin = TILE_MARGIN / n;
    const x0 = coords.x / n - margin;
    const y0 = coords.y / n - margin;
    const x1 = (coords.x + 1) / n + margin;
    const y1 = (coords.y + 1) / n + margin;
    const [ex0, ey0, ex1, ey1] = extent;
    return ex0 <= x1 && ex1 >= x0 && ey0 <= y1 && ey1 >= y0;
}

export class LayerExtents {
    constructor() {
        this._layers = null;
        // flag -> Map(layer -> extent), for the sources that flag gates.
        this._gated = new Map();
        this._generation = 0;
    }

    // Forget the current extents: until the next fetch lands, every tile is
    // requested.  Returns the generation a fetch started now must carry.
    invalidate() {
        this._generation++;
        this._layers = null;
        this._gated = new Map();
        return this._generation;
    }

    // Adopt a layer_extents response, unless an invalidate() happened after
    // the fetch it answers was issued.  Returns true when adopted.
    apply(generation, resp) {
        if (generation !== this._generation) {
            return false;
        }
        if (!resp || resp.supported !== true || !resp.layers
            || typeof resp.layers !== 'object') {
            this._layers = null;
            return false;
        }
        const gated = new Map();
        if (resp.gated && typeof resp.gated === 'object') {
            for (const [flag, layers] of Object.entries(resp.gated)) {
                if (layers && typeof layers === 'object') {
                    gated.set(flag, new Map(Object.entries(layers)));
                }
            }
        }
        this._layers = new Map(Object.entries(resp.layers));
        this._gated = gated;
        return true;
    }

    // Invalidate, then fetch through `request` (WebSocketManager.request).
    // Resolves to whether the reply was adopted; never rejects.
    refetch(request) {
        const generation = this.invalidate();
        let pending;
        try {
            pending = Promise.resolve(request({ type: 'layer_extents' }));
        } catch (_) {
            return Promise.resolve(false);
        }
        return pending
            .then((resp) => this.apply(generation, resp))
            .catch(() => false);
    }

    // False only when tile `coords` ({x, y, z}) of `layer` provably has
    // nothing to draw under `visibility`.
    mayHaveContent(layer, coords, visibility) {
        if (!this._layers || !this._layers.has(layer)) {
            return true;
        }
        if (visibility
            && NON_GEOMETRY_FLAGS.some((flag) => visibility[flag])) {
            return true;
        }
        if (tileOverlaps(this._layers.get(layer), coords)) {
            return true;
        }
        for (const [flag, layers] of this._gated) {
            if (visibility && visibility[flag] === false) {
                continue;
            }
            if (layers.has(layer) && tileOverlaps(layers.get(layer), coords)) {
                return true;
            }
        }
        return false;
    }
}

// Whether a tile layer should request `layer` at `coords`.  `ctx` is a tile
// layer's context ({ visibility, app }); app.layerExtents is absent until
// main.js installs it, and in that case everything is requested.
export function tileMayHaveContent(ctx, layer, coords) {
    const extents = ctx && ctx.app && ctx.app.layerExtents;
    if (!extents) {
        return true;
    }
    return extents.mayHaveContent(layer, coords, ctx.visibility);
}
