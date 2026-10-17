// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Pure geometry of the 2D layout view.
//
// World units are CSS px at zoom 0 with y up, so the tile grid is the square
// [0, T] x [0, T] for a tile size of T.  A view state is
// `{ target: [x, y], zoom, width, height }`, width and height in CSS px, and
// screen coordinates are CSS px from the view's top-left corner.  Tile rows are
// the server's: y counts down from the top of the grid.

const EPS = 1e-9;

function scaleOf(zoom) {
    return Math.pow(2, zoom);
}

export function project([wx, wy], vs) {
    const s = scaleOf(vs.zoom);
    return [(wx - vs.target[0]) * s + vs.width / 2,
            vs.height / 2 - (wy - vs.target[1]) * s];
}

export function unproject([px, py], vs) {
    const s = scaleOf(vs.zoom);
    return [vs.target[0] + (px - vs.width / 2) / s,
            vs.target[1] - (py - vs.height / 2) / s];
}

// The world rect [minX, minY, maxX, maxY] the view shows.
export function viewWorldRect(vs) {
    const [x0, y1] = unproject([0, 0], vs);
    const [x1, y0] = unproject([vs.width, vs.height], vs);
    return [x0, y0, x1, y1];
}

export function clampZoom(zoom, { minZoom = -Infinity, maxZoom = Infinity } = {}) {
    return Math.max(minZoom, Math.min(maxZoom, zoom));
}

// The view that fits world `rect` with `padding` px on every side: the zoom is
// floored to a whole level after rounding to 1/100 of a level, and the rect is
// centred.
export function fitView(rect, vs, { padding = [0, 0], minZoom, maxZoom } = {}) {
    const w = Math.max(1, vs.width - 2 * padding[0]);
    const h = Math.max(1, vs.height - 2 * padding[1]);
    const rw = Math.max(rect[2] - rect[0], EPS);
    const rh = Math.max(rect[3] - rect[1], EPS);
    const raw = Math.log2(Math.min(w / rw, h / rh));
    const zoom = clampZoom(Math.floor(Math.round(raw * 100) / 100),
                           { minZoom, maxZoom });
    return { target: [(rect[0] + rect[2]) / 2, (rect[1] + rect[3]) / 2], zoom };
}

// The target that keeps the world point under `screenPt` still at `zoom`.
export function zoomAround(vs, screenPt, zoom) {
    const [wx, wy] = unproject(screenPt, vs);
    const s = scaleOf(zoom);
    return [wx - (screenPt[0] - vs.width / 2) / s,
            wy + (screenPt[1] - vs.height / 2) / s];
}

// Tile level for a view zoom: floored, so tiles are only ever shown upscaled
// (the moiré fix of #10463).  While an animation heads for `targetZoom` and the
// new level stays hidden until it is complete, that level may load early.
export function tileZoomFor(zoom, { targetZoom = null, hidingNewLevels = false } = {}) {
    const z = (hidingNewLevels && targetZoom !== null) ? targetZoom : zoom;
    return Math.floor(z + EPS);
}

// Inclusive tile range at level z covering world `rect`, clamped to the 2^z
// grid; null when the rect misses the grid.
export function tileRange(rect, z, tileSize) {
    const n = Math.pow(2, z);
    const s = tileSize / n;
    const x0 = Math.max(0, Math.floor(rect[0] / s));
    const x1 = Math.min(n - 1, Math.ceil(rect[2] / s) - 1);
    const y0 = Math.max(0, n - Math.ceil(rect[3] / s));
    const y1 = Math.min(n - 1, n - 1 - Math.floor(rect[1] / s));
    if (x0 > x1 || y0 > y1) {
        return null;
    }
    return { z, x0, x1, y0, y1 };
}

// `rect` grown by `f` times its width and height on every side.
export function growRect([x0, y0, x1, y1], f) {
    const dx = (x1 - x0) * f;
    const dy = (y1 - y0) * f;
    return [x0 - dx, y0 - dy, x1 + dx, y1 + dy];
}

export function growRange(range, by = 1) {
    if (!range) {
        return null;
    }
    const n = Math.pow(2, range.z);
    return {
        z: range.z,
        x0: Math.max(0, range.x0 - by),
        x1: Math.min(n - 1, range.x1 + by),
        y0: Math.max(0, range.y0 - by),
        y1: Math.min(n - 1, range.y1 + by),
    };
}

export function inRange(range, x, y) {
    return !!range && x >= range.x0 && x <= range.x1
        && y >= range.y0 && y <= range.y1;
}

// Tiles of `range`, nearest the range centre first.
export function rangeTiles(range) {
    if (!range) {
        return [];
    }
    const cx = (range.x0 + range.x1) / 2;
    const cy = (range.y0 + range.y1) / 2;
    const out = [];
    for (let y = range.y0; y <= range.y1; y++) {
        for (let x = range.x0; x <= range.x1; x++) {
            out.push({ z: range.z, x, y, d: (x - cx) ** 2 + (y - cy) ** 2 });
        }
    }
    out.sort((a, b) => a.d - b.d);
    return out.map(({ z, x, y }) => ({ z, x, y }));
}

// World bounds [left, bottom, right, top] of tile (z, x, y).
export function tileWorldBounds(z, x, y, tileSize) {
    const s = tileSize / Math.pow(2, z);
    return [x * s, tileSize - (y + 1) * s, (x + 1) * s, tileSize - y * s];
}

export function rectsOverlap(a, b) {
    return a[0] < b[2] && b[0] < a[2] && a[1] < b[3] && b[1] < a[3];
}

// The target nudged so world (0, 0) lands on a whole device pixel.  Tile sizes
// are whole device pixels, so every tile edge then does too and a tile at rest
// maps 1:1 onto the device grid.
export function snapTarget(vs, dpr) {
    const s = scaleOf(vs.zoom) * dpr;
    const [px, py] = project([0, 0], vs);
    const rx = px * dpr - Math.round(px * dpr);
    const ry = py * dpr - Math.round(py * dpr);
    return [vs.target[0] + rx / s, vs.target[1] - ry / s];
}

// The van Wijk & Nuij zoom-and-pan path from view `a` to view `b` (the one
// d3.interpolateZoom draws): it pulls out, travels and closes in at a constant
// apparent speed.  Returns `at(t)` -> { target, zoom } for t in [0, 1] and the
// path length `S` (larger is a longer flight).
export function flyPath(a, b) {
    const rho = Math.SQRT2;
    const rho2 = 2;
    const rho4 = 4;
    const size = Math.max(a.width, a.height);
    const [ux0, uy0] = a.target;
    const [ux1, uy1] = b.target;
    const w0 = size / scaleOf(a.zoom);   // world units across the view
    const w1 = size / scaleOf(b.zoom);
    const dx = ux1 - ux0;
    const dy = uy1 - uy0;
    const d2 = dx * dx + dy * dy;
    const zoomOf = (w) => Math.log2(size / w);
    if (d2 < 1e-12) {
        const S = Math.log(w1 / w0) / rho;
        return {
            S: Math.abs(S),
            at: (t) => ({ target: [ux0 + t * dx, uy0 + t * dy],
                          zoom: zoomOf(w0 * Math.exp(rho * t * S)) }),
        };
    }
    const d1 = Math.sqrt(d2);
    const b0 = (w1 * w1 - w0 * w0 + rho4 * d2) / (2 * w0 * rho2 * d1);
    const b1 = (w1 * w1 - w0 * w0 - rho4 * d2) / (2 * w1 * rho2 * d1);
    const r0 = Math.log(Math.sqrt(b0 * b0 + 1) - b0);
    const r1 = Math.log(Math.sqrt(b1 * b1 + 1) - b1);
    const S = (r1 - r0) / rho;
    const coshr0 = Math.cosh(r0);
    return {
        S,
        at: (t) => {
            if (t >= 1) {
                return { target: [ux1, uy1], zoom: b.zoom };
            }
            const s = t * S;
            const u = w0 / (rho2 * d1)
                * (coshr0 * Math.tanh(rho * s + r0) - Math.sinh(r0));
            return { target: [ux0 + u * dx, uy0 + u * dy],
                     zoom: zoomOf(w0 * coshr0 / Math.cosh(rho * s + r0)) };
        },
    };
}
