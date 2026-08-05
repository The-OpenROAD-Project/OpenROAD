// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Shared UI utilities.

// True when the app was bootstrapped from a saved/static report
// (i.e. there is no live WebSocket backend).
export function isStaticMode(app) {
    return !!app?.websocketManager?.isStaticMode;
}

// --- Selection ownership ---
//
// Several panels can replace the selection: the canvas, the Inspector's links
// and prev/next, the Display Control's layer rows, the fanout chart, the
// schematic and the rulers.  The server runs its handlers on a thread pool, so
// two selections issued in quick succession can complete out of order and the
// slower — older — one would otherwise land last, leaving the Inspector
// describing an object the server no longer has selected.
//
// beginSelection() hands ownership to the caller and returns a token; the
// caller drops its own response once isCurrentSelection() goes false.  Panels
// that decorate their own selection register a reset callback so the new owner
// clears the previous one's decoration — the callback also runs for the panel
// that called beginSelection(), so paint after the call, never before.

export function beginSelection(app) {
    app.selectionToken = (app.selectionToken || 0) + 1;
    for (const reset of app.selectionResetters || []) {
        reset();
    }
    return app.selectionToken;
}

export function isCurrentSelection(app, token) {
    return app.selectionToken === token;
}

export function onSelectionReset(app, fn) {
    if (!app.selectionResetters) {
        app.selectionResetters = [];
    }
    app.selectionResetters.push(fn);
}

// Leaflet map options for the 2D layout viewer.
//
// zoomSnap:1 + zoomDelta:1 force the map to rest only at INTEGER zoom levels,
// so server tiles are displayed 1:1 at rest with no fractional CSS rescaling
// of the tile pane.  Downscaling a band-limited dense raster (bump arrays)
// re-introduces the moiré beat the server worked to remove; resting on integer
// zoom (combined with the floor _clampZoom override in the tile layers, which
// guarantees the pane is only ever upscaled) prevents that.
//
// `crs` is parameterized so this is unit-testable without the Leaflet global.
export function buildMapOptions(
    crs = (typeof L !== 'undefined' ? L.CRS.Simple : undefined)) {
    return {
        crs,
        zoom: 1,
        zoomSnap: 1,
        zoomDelta: 1,
        fadeAnimation: false,
        attributionControl: false,
    };
}

// Build a display-controls group header row: an expand/collapse triangle and
// a stretching name cell.  Callers fill in `name.textContent` and append their
// checkbox columns after it, which pins those columns to the row's right edge
// (see the .vis-name / .vis-cb rules in style.css).
//
// The header is a plain <div>, not a <label>: a <label> wrapping the
// visibility checkbox activates that checkbox for a click anywhere in the
// row, so any click that misses a checkbox toggles the whole category's
// visibility.  Here only the checkboxes toggle state; the triangle, the
// group name and the empty space expand/collapse, matching the Qt GUI's
// tree where clicking an item's name never changes visibility.
export function makeGroupHeader(className = 'vis-group-header') {
    const header = document.createElement('div');
    header.className = className;
    const arrow = document.createElement('span');
    arrow.className = 'vis-arrow';
    header.appendChild(arrow);
    const name = document.createElement('span');
    name.className = 'vis-name';
    header.appendChild(name);
    return { header, arrow, name };
}

// Wire `header` so clicking its triangle, its name cell, or its own bare area
// (the empty space, which targets `header` itself) expands or collapses
// `children`, keeping `arrow`'s glyph in sync.  `collapsed` is the initial
// state and is applied immediately.
//
// Only those three targets collapse.  Every other element in the row keeps its
// own click, so a control added later cannot both act and collapse the group:
// the worst a new child can do is not respond, which is visible immediately,
// rather than firing two actions at once.
//
// The name cell is listed explicitly because it is an element: the group name
// used to be a bare text node whose clicks reported `header` as the target,
// and moving it into a stretching .vis-name span (for column alignment) would
// otherwise have silently dropped the largest click target in the row.
export function attachGroupCollapse(header, arrow, children, collapsed) {
    const apply = (c) => {
        children.classList.toggle('collapsed', c);
        arrow.textContent = c ? '▶' : '▼';
    };
    apply(!!collapsed);
    header.addEventListener('click', (e) => {
        const isName = e.target && e.target.parentNode === header
            && e.target.classList
            && e.target.classList.contains('vis-name');
        if (e.target !== header && e.target !== arrow && !isName) return;
        apply(!children.classList.contains('collapsed'));
    });
}

// Make table column headers resizable by dragging.
// widths is an optional array of CSS width strings (e.g. saved from a
// previous render); when given, it is applied directly instead of
// measuring natural widths, avoiding a forced reflow.
export function makeResizableHeaders(table, widths) {
    const headers = table.querySelectorAll('thead th');
    if (!widths || !widths[0]) {
        // Reset to auto layout so browser computes natural column widths;
        // reading offsetWidth forces a reflow.
        table.style.tableLayout = 'auto';
        headers.forEach((th) => th.style.width = '');
        widths = Array.from(headers, (th) => th.offsetWidth + 'px');
    }
    // Lock in widths and switch to fixed layout
    headers.forEach((th, i) => th.style.width = widths[i] || '');
    table.style.tableLayout = 'fixed';

    headers.forEach((th, idx) => {
        if (idx === headers.length - 1) return; // skip last column
        const grip = document.createElement('div');
        grip.className = 'col-resize-grip';
        th.style.position = 'relative';
        th.appendChild(grip);

        let startX, startW;
        const onMouseMove = (e) => {
            th.style.width = Math.max(30, startW + e.clientX - startX) + 'px';
        };
        const onMouseUp = () => {
            document.removeEventListener('mousemove', onMouseMove);
            document.removeEventListener('mouseup', onMouseUp);
            // Releasing the drag fires a click on the common ancestor of the
            // press/release targets (often the th); swallow it so header
            // click handlers (e.g. sorting) don't trigger. The timeout
            // clears the suppressor when no click fires (e.g. release
            // outside the window).
            const swallowClick = (e) => e.stopPropagation();
            document.addEventListener('click', swallowClick,
                                      { capture: true, once: true });
            setTimeout(() => {
                document.removeEventListener('click', swallowClick,
                                             { capture: true });
            }, 0);
        };
        grip.addEventListener('mousedown', (e) => {
            e.preventDefault();
            startX = e.clientX;
            startW = th.offsetWidth;
            document.addEventListener('mousemove', onMouseMove);
            document.addEventListener('mouseup', onMouseUp);
        });
    });
}
