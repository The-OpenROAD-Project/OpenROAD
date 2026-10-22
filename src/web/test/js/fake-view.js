// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// A stand-in for layout-view.js: the same API, no renderer.  Records what the
// modules under test ask of the view.  Needs a DOM (setup-dom.js).

import { project, unproject } from '../../src/view-math.js';

export function makeFakeView({ width = 1000, height = 800, zoom = 0,
                               target = [128, 128] } = {}) {
    const container = document.createElement('div');
    const listeners = new Map();
    const vs = { width, height, zoom, target: [...target] };
    const limits = { minZoom: 0, maxZoom: 20 };

    const view = {
        vs,
        calls: [],             // [name, ...args] of every navigation call
        overlays: new Map(),   // id -> { shapes, order }
        images: new Map(),
        markers: new Map(),    // group -> { visible, items: Map id -> m }
        cursorModes: new Set(),
        slots: [],
        flashes: 0,

        getContainer: () => container,
        getSize: () => ({ width: vs.width, height: vs.height }),
        getZoom: () => vs.zoom,
        getTargetZoom: () => vs.zoom,
        getCenter: () => [...vs.target],
        getViewState: () => ({ ...vs, target: [...vs.target] }),
        isAnimating: () => false,
        isMoving: () => false,
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

        setLimits(l) {
            Object.assign(limits, l);
            view.calls.push(['setLimits', { ...l }]);
        },
        getLimits: () => ({ ...limits }),
        setView(center, z = vs.zoom, opts) {
            view.calls.push(['setView', [...center], z, opts]);
            vs.target = [...center];
            if (z !== vs.zoom) {
                vs.zoom = z;
                view.emit('zoom');
            }
            view.emit('move');
        },
        fitWorldRect(rect, opts) {
            view.calls.push(['fitWorldRect', [...rect], opts]);
        },
        zoomAround(pt, delta, opts) {
            view.calls.push(['zoomAround', pt, delta, opts]);
        },
        zoomIn() {
            view.calls.push(['zoomIn']);
        },
        zoomOut() {
            view.calls.push(['zoomOut']);
        },
        panByPx(dx, dy, opts) {
            view.calls.push(['panByPx', dx, dy, opts]);
        },
        // Change the zoom as a user would, firing the view's events.
        setZoomForTest(z) {
            vs.zoom = z;
            view.emit('zoom');
        },
        invalidateSize() {},
        applyPrefs() {},
        redraw() {},

        addTileSlot(id, opts = {}) {
            const slot = {
                id, opts, visible: opts.visible !== false, refreshes: 0,
                setVisible(v) { slot.visible = !!v; },
                isVisible: () => slot.visible,
                refresh() { slot.refreshes++; },
                remove() { slot.removed = true; },
            };
            view.slots.push(slot);
            return slot;
        },

        setOverlay(id, shapes, { order = 0 } = {}) {
            if (!shapes || shapes.length === 0) {
                view.overlays.delete(id);
            } else {
                view.overlays.set(id, { shapes, order });
            }
        },
        hasOverlay: (id) => view.overlays.has(id),
        shapes: (id) => view.overlays.get(id)?.shapes || [],
        setImageOverlay(id, img, { order = 0 } = {}) {
            if (!img) {
                view.images.delete(id);
            } else {
                view.images.set(id, { ...img, order });
            }
        },

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
        debugSnapshot: () => ({ zoom: vs.zoom, target: [...vs.target] }),
        destroy() {},
    };

    function markerGroup(group) {
        if (!view.markers.has(group)) {
            view.markers.set(group, { visible: true, items: new Map() });
        }
        return view.markers.get(group);
    }

    return view;
}
