// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';

import {
    deviceResidualCss, snapContainerToDeviceGrid, snapTileContainers,
    installDeviceGridSnapping,
} from '../../src/device-pixels.js';

// A tile boundary as the browser sees it, in device pixels. Whole => the two
// tiles either side abut on a pixel edge; fractional => both antialias into the
// same pixel column and the pane background shows between them.
function boundaryDevicePx(tileCssLeft, tilesAlong, dpr, tileSize = 256) {
    return (tileCssLeft + tilesAlong * tileSize) * dpr;
}

// On the grid to within floating-point exactness, which is the real guarantee:
// the correction is arithmetic on doubles, so it lands within an epsilon of a
// whole device pixel rather than exactly on one. A tenth of a nanopixel is not
// a seam.
function assertOnDeviceGrid(devicePx, message) {
    assert.ok(Math.abs(devicePx - Math.round(devicePx)) < 1e-9,
              `${message}: ${devicePx} is not a whole device pixel`);
}

// Stand-in for the DOM Leaflet builds: .leaflet-tile elements inside a
// .leaflet-tile-container (which Leaflet transforms) inside a .leaflet-layer
// (which it only ever sets a z-index on).
//
// `origin` is Leaflet's level.origin: _getTilePos places tile k at
// `k*tileSize - origin`, and origin is an arbitrary rounded integer, NOT a
// multiple of the tile size. Modelling it is the point — a container sitting
// exactly on the device grid with every tile inside it a quarter pixel off is
// precisely the case that has to be caught.
function fakeLayerDom(containerLeft, origin = 0, containerTop = 0) {
    const holder = { style: {}, className: 'leaflet-layer' };
    const container = {
        className: 'leaflet-tile-container',
        parentElement: holder,
        leafletLeft: containerLeft,
        leafletTop: containerTop,
        origin,
        querySelector(sel) {
            assert.equal(sel, '.leaflet-tile');
            return this.tile;
        },
    };
    container.tile = { className: 'leaflet-tile', parentElement: container };
    holder.child = container;
    return { holder, container, tile: container.tile };
}

// Where an element actually renders: Leaflet's placement plus whatever
// correction currently sits on the layer holder. This is what
// getBoundingClientRect would report, and reproducing the real chain — holder
// correction, container transform, per-tile origin offset — is what makes these
// tests mean anything.
function renderedRect(el) {
    const container = el.className === 'leaflet-tile' ? el.parentElement : el;
    const holder = container.parentElement;
    const tileOffset = el === container ? 0 : -container.origin;
    return {
        left: container.leafletLeft + tileOffset + (holder._orSnapDx || 0),
        top: container.leafletTop + tileOffset + (holder._orSnapDy || 0),
    };
}

describe('the seam this module exists to remove', () => {
    it('puts tile boundaries off the device grid at dpr 1.25', () => {
        // Leaflet rounds its offsets to whole CSS px, which at 1.25 is a
        // quarter pixel off the device grid for three of every four offsets.
        const fractional = [37, 38, 39].filter(
            (left) => !Number.isInteger(boundaryDevicePx(left, 3, 1.25)));
        assert.deepEqual(fractional, [37, 38, 39]);
        // 40 % 4 === 0, the one offset in four that already lands on the grid.
        assert.ok(Number.isInteger(boundaryDevicePx(40, 3, 1.25)));
    });

    it('lands every boundary on the grid once the layer is corrected', () => {
        // Every combination of container placement and level.origin: the
        // container alone was not enough, because the origin term is applied
        // to the tiles inside it.
        for (const left of [37, 38, 39, 40, -13, 0]) {
            for (const origin of [0, 1, 2, 3, 40, 8123, -57]) {
                const { container, tile } = fakeLayerDom(left, origin);
                snapContainerToDeviceGrid(container, 1.25, renderedRect);
                const corrected = renderedRect(tile).left;
                for (const along of [0, 1, 2, 17]) {
                    assertOnDeviceGrid(
                        boundaryDevicePx(corrected, along, 1.25),
                        `left ${left}, origin ${origin}, tile ${along}`);
                }
            }
        }
    });

    it('holds for every ratio a display reports, not just 1.25', () => {
        // 256*dpr is whole for all of these, so correcting the container is the
        // whole fix; the tile's own 256 CSS px box is already whole-device.
        for (const dpr of [1, 1.25, 1.5, 1.75, 2, 2.5, 3]) {
            assert.ok(Number.isInteger(256 * dpr), `256*${dpr} must be whole`);
            const { container, tile } = fakeLayerDom(37.4, 813, -8.9);
            snapContainerToDeviceGrid(container, dpr, renderedRect);
            const rect = renderedRect(tile);
            assertOnDeviceGrid(boundaryDevicePx(rect.left, 5, dpr), `x @${dpr}`);
            assertOnDeviceGrid(boundaryDevicePx(rect.top, 5, dpr), `y @${dpr}`);
        }
    });
});

describe('deviceResidualCss', () => {
    it('is less than half a device pixel, and signed', () => {
        for (const dpr of [1.25, 1.5, 2, 3]) {
            for (const pos of [0, 0.4, 12.7, -3.3, 1000.9]) {
                const residual = deviceResidualCss(pos, dpr);
                assert.ok(Math.abs(residual) * dpr <= 0.5 + 1e-9,
                          `dpr ${dpr}, pos ${pos} -> ${residual}`);
                assert.ok(Number.isInteger(
                    Math.round((pos - residual) * dpr * 1e6) / 1e6));
            }
        }
    });

    it('is zero for a position already on the grid', () => {
        assert.equal(deviceResidualCss(12, 1), 0);
        assert.equal(deviceResidualCss(40, 1.25), 0);
        assert.equal(deviceResidualCss(0, 1.25), 0);
    });

    it('is zero for input it cannot use', () => {
        assert.equal(deviceResidualCss(NaN, 2), 0);
        assert.equal(deviceResidualCss(10, 0), 0);
        assert.equal(deviceResidualCss(10, -1), 0);
    });
});

describe('snapContainerToDeviceGrid', () => {
    // The bug this whole module had in its first draft: correcting the element
    // Leaflet itself transforms would be overwritten on the next pan. Leaflet
    // owns .leaflet-tile-container; only .leaflet-layer is free.
    it('writes the correction on the layer, never on the container', () => {
        const { holder, container } = fakeLayerDom(37);
        snapContainerToDeviceGrid(container, 1.25, renderedRect);
        assert.ok(holder.style.transform.startsWith('translate3d('));
        assert.equal(container.style, undefined);
    });

    it('measures a tile, not the container', () => {
        // The container can be exactly on the grid while its tiles are not:
        // level.origin is applied per tile, inside the container.
        const { container, tile } = fakeLayerDom(40, 3);  // 40*1.25 is whole
        assert.ok(Number.isInteger(40 * 1.25));
        snapContainerToDeviceGrid(container, 1.25, renderedRect);
        assertOnDeviceGrid(renderedRect(tile).left * 1.25, 'tile');
    });

    it('does nothing for a container with no tiles yet', () => {
        const { holder, container } = fakeLayerDom(37);
        container.tile = null;
        assert.equal(snapContainerToDeviceGrid(container, 1.25, renderedRect),
                     null);
        assert.equal(holder.style.transform, undefined);
    });

    it('converges in one step and does not drift when re-run', () => {
        const { holder, container } = fakeLayerDom(37, 813);
        const first = snapContainerToDeviceGrid(container, 1.25, renderedRect);
        const applied = holder.style.transform;
        for (let i = 0; i < 5; i++) {
            const again = snapContainerToDeviceGrid(container, 1.25,
                                                    renderedRect);
            assert.equal(again.dx, first.dx);
            assert.equal(holder.style.transform, applied);
        }
    });

    it('follows Leaflet when it moves the container', () => {
        const { container, tile } = fakeLayerDom(37, 813);
        snapContainerToDeviceGrid(container, 1.25, renderedRect);
        container.leafletLeft = 38;  // a one-CSS-pixel pan
        snapContainerToDeviceGrid(container, 1.25, renderedRect);
        assertOnDeviceGrid(renderedRect(tile).left * 1.25, 'after pan');
    });

    it('clears the transform when nothing needs correcting', () => {
        // 40 and origin 0: container and tiles both already on the grid.
        const { holder, container } = fakeLayerDom(40, 0);
        snapContainerToDeviceGrid(container, 1.25, renderedRect);
        assert.equal(holder.style.transform, '');
    });

    it('does nothing for a container with no parent', () => {
        assert.equal(snapContainerToDeviceGrid(null, 1.25, renderedRect), null);
        assert.equal(
            snapContainerToDeviceGrid({ parentElement: null }, 1.25,
                                      renderedRect),
            null);
    });
});

describe('snapTileContainers', () => {
    function fakeMap(lefts) {
        const doms = lefts.map((left, i) => fakeLayerDom(left, 800 + i));
        return {
            doms,
            getPane(name) {
                assert.equal(name, 'tilePane');
                return { querySelectorAll: () => doms.map((d) => d.container) };
            },
        };
    }

    it('corrects every tile layer on the map', () => {
        const map = fakeMap([37, 38, 39]);
        assert.equal(snapTileContainers(map, 1.25, renderedRect), 3);
        for (const { tile } of map.doms) {
            assertOnDeviceGrid(renderedRect(tile).left * 1.25, 'layer');
        }
    });

    it('skips the work entirely at an integer ratio', () => {
        // Whole CSS px are whole device px there, so every residual is zero;
        // this runs on each frame of a drag and should cost nothing.
        const map = fakeMap([37, 38]);
        for (const dpr of [1, 2, 3]) {
            assert.equal(snapTileContainers(map, dpr, renderedRect), 0);
            assert.equal(map.doms[0].holder.style.transform, undefined);
        }
    });

    it('survives a map with no tile pane yet', () => {
        assert.equal(snapTileContainers(null, 1.25, renderedRect), 0);
        assert.equal(snapTileContainers({}, 1.25, renderedRect), 0);
        assert.equal(
            snapTileContainers({ getPane: () => null }, 1.25, renderedRect), 0);
    });
});

describe('installDeviceGridSnapping', () => {
    it('re-snaps after a move, a zoom and a resize', () => {
        const handlers = [];
        const map = {
            on(events, fn) { handlers.push({ events, fn }); },
            getPane: () => null,
        };
        assert.equal(installDeviceGridSnapping(map, () => 1.25), true);
        assert.equal(handlers.length, 1);
        for (const event of ['move', 'zoomend', 'viewreset', 'resize']) {
            assert.ok(handlers[0].events.split(' ').includes(event), event);
        }
    });

    it('reads the ratio per call, so moving to another monitor is picked up',
       () => {
           const seen = [];
           const map = { on() {}, getPane: () => { seen.push(1); return null; } };
           let dpr = 1;
           installDeviceGridSnapping(map, () => dpr);
           assert.equal(seen.length, 0);  // integer ratio: no measuring at all
           dpr = 1.25;
           installDeviceGridSnapping(map, () => dpr);
           assert.equal(seen.length, 1);
       });

    it('declines a map it does not recognize', () => {
        assert.equal(installDeviceGridSnapping(null), false);
        assert.equal(installDeviceGridSnapping({}), false);
    });
});
