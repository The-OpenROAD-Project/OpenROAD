// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// A stand-in for layout-view.js: the part of its API the modules under test
// use, with no renderer.  Records what they ask of the view.  Needs a DOM
// (setup-dom.js).

import { project, unproject } from '../../src/view-math.js';

export function makeFakeView({ width = 1000, height = 800, zoom = 0,
                               target = [128, 128] } = {}) {
    const container = document.createElement('div');
    const listeners = new Map();
    const vs = { width, height, zoom, target: [...target] };

    const view = {
        calls: [],             // [name, ...args] of every recorded call
        overlays: new Map(),   // id -> shapes
        markers: new Map(),    // group -> { visible, items: Map id -> m }
        cursorModes: new Set(),
        flashes: 0,

        getContainer: () => container,
        getZoom: () => vs.zoom,
        getCenter: () => [...vs.target],
        project: (w) => project(w, vs),
        unproject: (p) => unproject(p, vs),

        on(types, fn) {
            for (const t of types.split(/\s+/)) {
                if (!listeners.has(t)) {
                    listeners.set(t, new Set());
                }
                listeners.get(t).add(fn);
            }
            return view;
        },
        off(types, fn) {
            for (const t of types.split(/\s+/)) {
                listeners.get(t)?.delete(fn);
            }
            return view;
        },
        emit(type, arg) {
            for (const fn of listeners.get(type) || []) {
                fn(arg);
            }
        },

        setView(center, z = vs.zoom, opts) {
            view.calls.push(['setView', [...center], z, opts]);
            vs.target = [...center];
            if (z !== vs.zoom) {
                vs.zoom = z;
                view.emit('zoom');
            }
        },
        fitWorldRect(rect, opts) {
            view.calls.push(['fitWorldRect', [...rect], opts]);
        },
        zoomIn() {
            view.calls.push(['zoomIn']);
        },
        zoomOut() {
            view.calls.push(['zoomOut']);
        },
        // Change the zoom as a user would, firing the view's events.
        setZoomForTest(z) {
            vs.zoom = z;
            view.emit('zoom');
        },

        addTileSlot(id, opts = {}) {
            const slot = {
                opts, refreshes: 0,
                refresh() { slot.refreshes++; },
            };
            return slot;
        },

        setOverlay(id, shapes) {
            if (!shapes || shapes.length === 0) {
                view.overlays.delete(id);
            } else {
                view.overlays.set(id, shapes);
            }
        },
        shapes: (id) => view.overlays.get(id) || [],

        addMarker(group, id, world, el, { center = false } = {}) {
            const g = markerGroup(group);
            g.items.get(id)?.el.remove();
            el.classList.add('or-view-marker');
            container.appendChild(el);
            g.items.set(id, { el, world: [...world], center });
            return el;
        },
        moveMarker(group, id, world) {
            const m = markerGroup(group).items.get(id);
            if (m) {
                m.world = [...world];
            }
        },
        clearMarkers(group) {
            const g = markerGroup(group);
            for (const m of g.items.values()) {
                m.el.remove();
            }
            g.items.clear();
        },
        setMarkerGroupVisible(group, visible) {
            markerGroup(group).visible = !!visible;
        },
        markersIn: (group) => [...markerGroup(group).items.values()],

        setCursorMode(mode, on = true) {
            if (on) {
                view.cursorModes.add(mode);
            } else {
                view.cursorModes.delete(mode);
            }
            container.classList.toggle(`${mode}-mode-cursor`, !!on);
        },
        flashTiles() {
            view.flashes++;
        },
        whenTilesSettled() {
            view.calls.push(['whenTilesSettled']);
            return Promise.resolve();
        },
        captureCanvas: async () => (view.canvas
            ? { image: view.canvas, rect: view.canvas.getBoundingClientRect() }
            : null),
    };

    function markerGroup(group) {
        if (!view.markers.has(group)) {
            view.markers.set(group, { visible: true, items: new Map() });
        }
        return view.markers.get(group);
    }

    return view;
}
