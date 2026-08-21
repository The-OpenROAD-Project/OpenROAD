// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Keep the tile grid on whole device pixels.
//
// Leaflet lays everything out in whole CSS pixels: Map._getNewPixelOrigin and
// GridLayer._setZoomTransform both .round() their offsets. A whole CSS pixel is
// NOT a whole device pixel unless devicePixelRatio is an integer — at dpr 1.25
// an odd CSS offset lands on a quarter of a pixel — so a tile's box gets its
// edges resampled into the device grid. Two neighbouring tiles then each paint
// partial coverage into the same device pixel column, and src-over compositing
// of two partial alphas does not add up to one: the pane background shows
// through. That is the dark hairline along every tile boundary. It explains why
// the seams come and go as you pan (at dpr 1.25 one offset in four is already
// exact) and why they reproduce on both the merged-canvas and the legacy <img>
// path — this is the browser's layout, not anything either path draws.
//
// Overlapping the tiles instead would be worse: routing layers render at 0.7
// opacity, so an overlapping strip blends twice and a dark seam becomes a bright
// one. They have to abut exactly, which is what landing on the grid gives.
//
// Why this measures the DOM rather than patching Leaflet: leaflet-src.js keeps
// setTransform/setPosition as closure-local functions and only publishes
// references to them on L.DomUtil (`exports.DomUtil = DomUtil`, at the end of
// the bundle). GridLayer._setZoomTransform and the map's pan call the closure
// originals, so assigning to L.DomUtil.setTransform patches nothing that
// Leaflet itself goes through. The placement can be corrected, not intercepted.
//
// So: read where Leaflet actually put the tile container — .leaflet-tile-container,
// the element it transforms — and put the compensating offset on that container's
// parent, .leaflet-layer, which Leaflet only ever writes a z-index to. Neither
// side overwrites the other, and the correction survives every update Leaflet
// makes to the child.
//
// This removes the shared PHASE. The other half is the PITCH: tiles must also be
// a whole number of device pixels apart, or only every 1/frac-th boundary can be
// on the grid whatever the phase is — see TILE_SIZE_CSS in tile-request.js,
// which is why the tile size is 240 rather than 256.

import { nativeDpr } from './tile-request.js';

// The element Leaflet positions, and the parent it leaves alone.
const TILE_CONTAINER = '.leaflet-tile-container';
// What actually has to land on the grid.
const TILE = '.leaflet-tile';

// CSS pixels to subtract from `css_pos` to land it on a device pixel. Signed,
// and always less than half a device pixel.
export function deviceResidualCss(css_pos, dpr) {
    if (!Number.isFinite(css_pos) || !(dpr > 0)) {
        return 0;
    }
    const device = css_pos * dpr;
    return (device - Math.round(device)) / dpr;
}

// Correct one tile container by offsetting its parent.
//
// Measured on a TILE, not on the container. Leaflet's _getTilePos puts a tile at
// `coords*tileSize - level.origin`, and level.origin is an arbitrary rounded
// integer rather than a multiple of the tile size — so the container can sit
// exactly on the device grid while every tile inside it is a quarter pixel off.
// A tile's rect carries the whole chain: map pane, container transform, this
// correction, and that origin term.
//
// One tile per container is enough: tiles differ from each other by whole
// multiples of the tile size, and that size is chosen to be a whole number of
// device pixels at every real ratio (TILE_SIZE_CSS in tile-request.js).
//
// The measured rect already includes whatever correction is on the parent now,
// so the new correction is the old one minus what is still left over. That
// converges in one step and never accumulates drift.
//
// `rectOf` is injected so this is testable without a layout engine, and
// defaults to the real measurement for every other caller.
export function snapContainerToDeviceGrid(container, dpr,
                                          rectOf = defaultRectOf) {
    const holder = container && container.parentElement;
    if (!holder) {
        return null;
    }
    // An empty container has no seams to fix, and nothing to measure: a level
    // gets its tiles during the same move/zoom that creates it, so the next
    // event corrects it.
    const tile = typeof container.querySelector === 'function'
        ? container.querySelector(TILE)
        : null;
    if (!tile) {
        return null;
    }
    const rect = rectOf(tile);
    const dx = (holder._orSnapDx || 0) - deviceResidualCss(rect.left, dpr);
    const dy = (holder._orSnapDy || 0) - deviceResidualCss(rect.top, dpr);
    holder._orSnapDx = dx;
    holder._orSnapDy = dy;
    // Written on the PARENT: the container's own transform belongs to Leaflet,
    // which rewrites it on every pan and zoom.
    holder.style.transform
        = (dx || dy) ? `translate3d(${dx}px, ${dy}px, 0)` : '';
    return { dx, dy };
}

// Correct every tile container under the map's tile pane.
//
// Rects are read for all of them before any style is written: interleaving
// reads and writes would force a layout flush per container, and this runs on
// every frame of a drag.
export function snapTileContainers(map, dpr, rectOf = defaultRectOf) {
    // At an integer ratio a whole CSS pixel IS a whole device pixel, so every
    // residual is zero and there is nothing to correct — skip the measuring
    // entirely rather than pay for it on every move.
    if (!map || typeof map.getPane !== 'function' || !(dpr > 0)
        || Number.isInteger(dpr)) {
        return 0;
    }
    const pane = map.getPane('tilePane');
    if (!pane || typeof pane.querySelectorAll !== 'function') {
        return 0;
    }
    let snapped = 0;
    // Read every rect before writing any style: interleaving them forces a
    // layout flush per container, and this runs on every frame of a drag.
    const measured = [...pane.querySelectorAll(TILE_CONTAINER)].map(
        (container) => {
            const tile = container.querySelector(TILE);
            return { container, rect: tile ? rectOf(tile) : null };
        });
    for (const { container, rect } of measured) {
        if (rect && snapContainerToDeviceGrid(container, dpr, () => rect)) {
            snapped++;
        }
    }
    return snapped;
}

function defaultRectOf(el) {
    return el.getBoundingClientRect();
}

// Re-snap whenever Leaflet has moved the tiles.
//
// `zoomend` rather than `zoom`: during the zoom animation the container carries
// a CSS transition and a scale, and correcting mid-flight would fight it for no
// benefit — the frames are in motion and a quarter pixel is invisible there.
const SNAP_EVENTS = 'move zoomend viewreset resize load';

export function installDeviceGridSnapping(map, dprOf = nativeDpr) {
    if (!map || typeof map.on !== 'function') {
        return false;
    }
    const snap = () => snapTileContainers(map, dprOf());
    map.on(SNAP_EVENTS, snap);
    snap();
    return true;
}
